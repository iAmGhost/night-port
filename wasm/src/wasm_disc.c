/* wasm_disc.c -- the browser host's CD-DA byte source: HTTP Range reads behind the cd_iso.h remote
 * seam.  docs/457, docs/460.
 *
 * WHAT THIS IS
 * ------------
 * The desktop host reads CD-DA from extracted/cd/trackNN.bin with fopen/fseek/fread; the Pocket host
 * reads the same sectors from APF data slots (pocket/src/night/of_disc.c). A browser has neither, and
 * it must not download 604 MB of tracks to hear the one song a scene plays. So this file answers
 * cd_iso.c's two remote callbacks -- "how long is track N", "give me 2352 bytes at an offset" -- with
 * Range requests, and keeps a sliding read-ahead window so the pump asks the network something like
 * once every 3.4 s of music instead of 75 times a second.
 *
 * THE ONE RULE THIS FILE IS BUILT AROUND: the network never stops the game.
 * ------------------------------------------------------------------------
 * These callbacks run inside the audio pump, which runs inside the game's own busy-wait, so *awaiting*
 * a fetch is not a free yield: with -sASYNCIFY the game's stack unwinds and the game's clock -- ticks,
 * present, the lot -- stops until the bytes land. Measured with a 250 ms round trip in front of the
 * reads (the harness can put one there now, NIGHT_NET=latency=250): one awaited read per 1.7 s window,
 * 260-300 ms of the pump's gap spent inside a fetch, 21 frames/s with a hitch every second and a half.
 * That is the reported "music streaming makes the game stutter", and it is why read-ahead is now a
 * request the game fires and forgets, not a stop it makes on the way to the next sector.
 *
 * The other reported symptom -- "it stops playing after a while and never recovered" -- was this file's
 * failure handling. One Range read that got no answer ended the track for the rest of the session: the
 * read failed, `cdda_pump` set `g_cdda_sectors = 0`, and nothing but the game's next fn_10b1d can play
 * a track again. A status that was not 206 was worse: it latched `g_range_unsupported` for the whole
 * page, so one 404, one 500, one captive portal muted CD-DA forever. A network that blinks is now a
 * hole in the music and nothing else: the sectors with no bytes are answered with silence, the reader
 * keeps the clock honest, and the asks come back after a backoff -- so the first byte that does arrive
 * resumes the song at the second it is supposed to be at.
 *
 * WHAT COSTS WHAT
 * ---------------
 * A window is 128 raw frames (301 056 B, 3.42 s of Red Book audio), two of them, in this file's own
 * static memory -- 602 KB, which is 0.7% of what one track is. The read-ahead starts when 64 sectors
 * (0.85 s) of a window are left, so the ask goes out while there is still more than half a second of
 * decoded samples in the ring, and 3.4 s of raw bytes in the window.
 *
 * A track the server does not have is REFUSED, exactly like a missing trackNN.bin: cd_iso marks it
 * absent and denies its reads, so it plays as silence rather than as invented bytes. A server that
 * answers a Range request with `200 OK` and the whole file is treated the same way (said once, in the
 * log): that is a server this build cannot stream from at all, and letting it through would trade a
 * 300 KB read-ahead for an 87 MB download. Note the distinction from everything else -- a 404, a 500,
 * a timeout and a dead socket are one window's worth of nothing, not a verdict on the server.
 */
#include "wasm_disc.h"

#include "cd/cd_iso.h"

#include <emscripten.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* 128 raw frames = 301 056 B = 3.42 s of Red Book audio per request. */
#define NK_DISC_WINDOW       128u
#define NK_DISC_WINDOW_BYTES (NK_DISC_WINDOW * 2352u)
#define NK_DISC_FRAME        2352u
/* Start the next fetch while this many sectors are still buffered behind the read pointer. */
#define NK_DISC_LEAD_SECTORS 64u
/* How long the game will stand still for bytes it has no other way to get. A track's first window is
 * the cold case: the ring is empty, so waiting costs a picture hitch and buys the whole of the music's
 * opening. Mid-stream the bytes were asked for most of a second ago; if they are not here within this
 * bound the network is the problem, not the waiting, and silence costs the game nothing. */
#define NK_DISC_WAIT_COLD_US   1500000ull
#define NK_DISC_WAIT_US          200000ull
/* Do not ask the network again for this long after a window got nothing. Without it a dead connection
 * costs one Range request per sector -- 75 a second, each one a rejected promise. */
#define NK_DISC_BACKOFF_US      1000000ull

/* A window's state. The BYTES live in this file's static buffer; what the browser's JS owns is the
 * request, and it writes into that buffer only while holding a token that still matches the request it
 * was started with (see nkjs_start), in one synchronous block. */
enum { NK_WIN_EMPTY = 0, NK_WIN_FILLING, NK_WIN_READY, NK_WIN_EOF };

typedef struct {
    uint32_t track;          /* 0 = empty */
    uint64_t off;            /* byte offset of the window's first byte */
    uint32_t want;           /* bytes asked for */
    uint32_t len;            /* bytes in it, once st == NK_WIN_READY */
    uint32_t tok;            /* the JS request this window is waiting on */
    int      eof_short;      /* the fetch that filled it came back short: the track ends here */
    int      st;
} nk_window;

static nk_window g_win[2];
static uint8_t   g_win_bytes[2][NK_DISC_WINDOW_BYTES];

static char     g_base[512]; /* ".../data/cd" -- track files are <base>/trackNN.bin */
static uint32_t g_reads, g_read_bytes, g_fetches, g_fetch_bytes, g_fetch_fail;
static uint32_t g_silence;                 /* bytes answered with silence for want of bytes */
static uint32_t g_wait, g_wait_max_us;     /* what a read that had to wait cost the GAME */
static uint64_t g_retry_after_us;          /* the backoff after a window got nothing */
static uint64_t g_sil_log_us;              /* one silence line a second, not one a sector */
static int      g_warm_wait = 1;           /* stop standing still for warm reads once one has timed out */
static int      g_range_unsupported;

/* The byte length of each track, remembered from the HEAD cd_iso asks for when it opens the track. It is
 * here so the read-ahead can be clamped: a window that starts inside the last 128 sectors of a track asks
 * for bytes the file does not have, and this repo's own server answers that with a closed connection --
 * see nkjs_start. Measured: every long run died at the last sector of the show's track. */
#define NK_DISC_TRACKS 64u
static uint64_t g_track_bytes[NK_DISC_TRACKS];

/* The same clock the host's cost numbers use (main_wasm.c's nk_now_us), so the cost of a wait and the
 * pump's max_gap are directly comparable. */
static uint64_t disc_now_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
    return 0;
}

/* ---- the browser side -----------------------------------------------------------------------------
 * The request belongs to JS and the bytes belong to C, and the only thing crossing between them is a
 * token. `nkjs_start` returns at once: the game does not stop for it. `nkjs_poll` answers for a token
 * without stopping anything, and `nkjs_wait` is the one call that does stop the game -- used only for a
 * window the game is reading right now.
 *
 * What the two pollers answer:
 *      >= 0        the bytes are in the buffer (their count; short means the file ended)
 *      NKJS_GO      in flight
 *      NKJS_NONE    no such request -- JS gave the slot to somebody else
 *      NKJS_NET     no answer at all: a rejected fetch (Wi-Fi gone, ERR_EMPTY_RESPONSE, socket closed)
 *      NKJS_HTTP    answered, but not with 206: a 404, a 500, a portal. One window's worth of nothing.
 *      NKJS_200     answered with 200 and no Range: the server does not do Range. The fatal one.
 *
 * Without the try/catch a rejected promise unwinds through the game's own stack -- measured: frames
 * stop, the rAF gap reaches 20 s, the run is over (docs/459).
 */
#define NKJS_GO    1
#define NKJS_NONE (-2)
#define NKJS_NET  (-3)
#define NKJS_HTTP (-4)
#define NKJS_200  (-5)

EM_JS(int, nkjs_start, (const char *url, uint32_t off, uint32_t len, uint8_t *dst, uint32_t slot), {
    const D = globalThis.__nkDisc || (globalThis.__nkDisc = { s: [], tok: 1 });
    let s = D.s[slot];
    if (!s) s = D.s[slot] = { busy: 0, tok: 0, off: 0, len: 0, got: -1, err: 0, p: null };
    /* The same ask is already running: hand back its token rather than asking twice. */
    if (s.busy && s.off === off && s.len === len) return s.tok;
    const tok = D.tok++;
    s.busy = 1; s.tok = tok; s.off = off; s.len = len; s.got = -1; s.err = 0;
    const myUrl = UTF8ToString(url);
    s.p = fetch(myUrl, { headers: { 'Range': 'bytes=' + off + '-' + (off + len - 1) } })
        .then(async (r) => {
            if (s.tok !== tok) return;                        /* superseded: these bytes are nobody's */
            if (r.status === 200) { s.err = -5; s.busy = 0; return; }   /* Range ignored */
            if (r.status !== 206) { s.err = -4; s.busy = 0; return; }
            const body = new Uint8Array(await r.arrayBuffer());
            if (s.tok !== tok) return;                        /* and again, after the body */
            const n = Math.min(body.length, len);
            HEAPU8.set(body.subarray(0, n), dst);             /* one synchronous block: C cannot be
                                                               * reading this window while it runs */
            s.got = n; s.err = 0; s.busy = 0;
        })
        .catch(() => { if (s.tok === tok) { s.err = -3; s.got = -1; s.busy = 0; } });
    return tok;
});

/* Answer for `tok` without waiting. `busy` is JS's own flag, cleared by the completion that wrote the
 * bytes, so a >= 0 answer means the bytes are where C was told they would be. */
EM_JS(int, nkjs_poll, (uint32_t slot, uint32_t tok), {
    const D = globalThis.__nkDisc || (globalThis.__nkDisc = { s: [], tok: 1 });
    const s = D.s[slot];
    if (!s || s.tok !== tok) return -2;
    if (s.busy) return 1;
    return s.err ? s.err : s.got;
});

/* The one call that stops the game. Bounded by `ms` so a socket the browser never lets go of cannot stop
 * it forever; a timeout is answered as the network error, which is what it is. */
EM_ASYNC_JS(int, nkjs_wait, (uint32_t slot, uint32_t tok, uint32_t ms), {
    const D = globalThis.__nkDisc || (globalThis.__nkDisc = { s: [], tok: 1 });
    const s = D.s[slot];
    if (!s || s.tok !== tok) return -2;
    if (s.busy && s.p) {
        await Promise.race([s.p, new Promise((r) => setTimeout(r, ms))]);
        if (s.busy && s.tok === tok) { s.busy = 0; s.err = -3; s.got = -1; }   /* it never answered */
    }
    return s.err ? s.err : s.got;
});

/* HEAD for the file's length, once. 0 = answered (0 in *out32 means "present, length unknown"), -1 = not
 * there. nk_disc_size below is what retries. */
EM_ASYNC_JS(int, nkjs_head_size, (const char *url, uint32_t *out32), {
    let r;
    try { r = await fetch(UTF8ToString(url), { method: 'HEAD' }); } catch (e) { return -1; }
    if (!r.ok) return -1;
    const cl = r.headers.get('content-length');
    HEAPU32[out32 >> 2] = cl ? parseInt(cl, 10) : 0;   /* no content-length: present, length unknown */
    return 0;
});

EM_ASYNC_JS(int, nkjs_sleep, (uint32_t ms), { await new Promise((r) => setTimeout(r, ms)); return 0; });

void nk_disc_base(const char *base) { snprintf(g_base, sizeof g_base, "%s", base ? base : ""); }

static void track_url(uint32_t track, char *out, size_t n) {
    snprintf(out, n, "%s/track%02u.bin", g_base, track);
}

/* HEAD for the file's length. 0 = answered, -1 = not there.
 *
 * Tried three times, because this is the one answer the port cannot take back: cd_iso marks a track it
 * cannot size permanently absent (g_track_absent, cleared only by a re-open), so a HEAD lost to one blip
 * at boot mutes that track for the whole session -- the "and it never came back" symptom with nothing
 * for the player to have done. 200 ms and 800 ms between tries, in the boot window where nobody is
 * playing yet. */
int nk_disc_size(uint32_t track, uint64_t *out_bytes) {
    char url[600];
    track_url(track, url, sizeof url);
    const uint32_t backoff_ms[] = { 0, 200, 800 };
    for (unsigned t = 0; t < 3; t++) {
        if (backoff_ms[t]) nkjs_sleep(backoff_ms[t]);
        uint32_t bytes = 0;
        if (nkjs_head_size(url, &bytes) == 0) {
            *out_bytes = (uint64_t)bytes;
            if (track < NK_DISC_TRACKS) g_track_bytes[track] = *out_bytes;   /* 0 = no content-length */
            if (t) printf("disc: track %u answered its size on try %u\n", track, t + 1u);
            return 0;
        }
    }
    printf("disc: track %u did not answer a HEAD in three tries -- cd_iso will refuse its reads\n", track);
    return -1;
}

/* Start a window. 0 = asked (adopt it with poll_window), -1 = there is nothing to ask for, because the
 * offset is past the last byte of the track. */
static int start_window(int slot, uint32_t track, uint64_t start, uint32_t want) {
    nk_window *w = &g_win[slot];
    uint64_t limit = (track < NK_DISC_TRACKS) ? g_track_bytes[track] : 0ull;
    if (limit && start >= limit) {
        w->track = track; w->off = start; w->want = 0; w->len = 0; w->eof_short = 1; w->st = NK_WIN_EOF;
        return -1;
    }
    if (limit && start + want > limit) want = (uint32_t)(limit - start);
    char url[600];
    track_url(track, url, sizeof url);
    w->tok   = (uint32_t)nkjs_start(url, (uint32_t)start, want, g_win_bytes[slot], (uint32_t)slot);
    w->track = track; w->off = start; w->want = want; w->len = 0; w->eof_short = 0; w->st = NK_WIN_FILLING;
    return 0;
}

/* Adopt whatever the browser has done with a filling window. Returns its state. */
static int poll_window(int slot) {
    nk_window *w = &g_win[slot];
    if (w->st != NK_WIN_FILLING) return w->st;
    int r = nkjs_poll(slot, w->tok);
    if (r == NKJS_GO) return NK_WIN_FILLING;
    if (r == NKJS_NONE) { w->st = NK_WIN_EMPTY; w->track = 0; w->len = 0; return NK_WIN_EMPTY; }
    if (r < 0) {
        if (r == NKJS_200) g_range_unsupported = 1;    /* the only failure that says "this server, never" */
        w->st = NK_WIN_EOF; w->len = 0;                /* nothing to read AT THIS OFFSET */
        g_fetch_fail++;
        return NK_WIN_EOF;
    }
    w->len = (uint32_t)r;
    w->st  = NK_WIN_READY;
    if (w->len) { g_fetches++; g_fetch_bytes += w->len; }
    if (w->len < w->want) w->eof_short = 1;            /* the file ended inside the window */
    return NK_WIN_READY;
}

/* Cover-check: does this window, as it stands, answer [off, off+n)? */
static int covers(const nk_window *w, uint32_t track, uint64_t off, uint64_t n) {
    return w->st == NK_WIN_READY && w->track == track && off >= w->off && off + n <= w->off + w->len;
}

/* Which slot to fill. A window that would cover `start` for this track is worth more than one that is
 * merely full, and a window that is still filling is worth more than one that is finished and useless. */
static int pick_slot(uint32_t track, uint64_t start) {
    int use[2];
    for (int i = 0; i < 2; i++) {
        const nk_window *w = &g_win[i];
        int would_cover = (w->track == track && w->st != NK_WIN_EOF && w->off <= start &&
                           start < w->off + (w->st == NK_WIN_READY ? w->len : w->want));
        use[i] = would_cover ? 2 : (w->st == NK_WIN_FILLING ? 1 : 0);
    }
    if (use[0] != use[1]) return use[0] < use[1] ? 0 : 1;
    return (g_win[0].off <= g_win[1].off) ? 0 : 1;
}

/* The answer when there are no bytes: silence, which is what an empty ring plays and what the desktop
 * host plays for a track it cannot open. Counted, because a player hears it. */
static void fill_silence(void *dst, uint32_t n) {
    memset(dst, 0, n);
    g_reads++; g_read_bytes += n; g_silence += n;
}

int nk_disc_pread(uint32_t track, uint64_t byte_off, void *dst, uint32_t n) {
    if (n == 0) return 0;
    if (g_range_unsupported) return -1;

    for (int i = 0; i < 2; i++) poll_window(i);           /* nothing in flight stays invisible */

    int cur = -1;
    for (int i = 0; i < 2; i++) if (covers(&g_win[i], track, byte_off, n)) cur = i;
    if (cur < 0) {
        uint64_t start = (byte_off / NK_DISC_FRAME) * NK_DISC_FRAME;
        uint64_t want  = NK_DISC_WINDOW_BYTES;
        if (byte_off + n > start + want) want = byte_off + n - start;   /* never under-read the request */

        /* Cold = this track has never been read at all, so nothing is queued to play. The ring is empty
         * whether we wait or not, and waiting is the difference between the music starting on its first
         * sample and losing its first second to silence. */
        int cold = (g_win[0].track != track && g_win[1].track != track);

        /* Is a request already on its way that can answer this -- this sector, or one of the 127 behind
         * it? If so it is the ask; do not make another one. Skipping this question is what made a stall
         * expensive instead of merely silent: with the reader walking on one sector a pump and the ring
         * empty, every pump asked for a new 300 KB window 2 352 B further on, evicted the window that was
         * on its way, and threw the answer away when it landed (stale=true, once per response). */
        cur = -1;
        for (int i = 0; i < 2; i++)
            if (g_win[i].st == NK_WIN_FILLING && g_win[i].track == track && g_win[i].off <= start &&
                start < g_win[i].off + g_win[i].want) cur = i;
        uint64_t bound;
        if (cur >= 0) {
            /* Cold is the only case worth standing still for here: the ring is empty either way. */
            bound = cold ? NK_DISC_WAIT_COLD_US : 0ull;
        } else if (disc_now_us() < g_retry_after_us) {   /* the network is down: keep time honest */
            if (disc_now_us() >= g_sil_log_us) {              /* once a second, not once a sector */
                g_sil_log_us = disc_now_us() + 1000000ull;
                printf("disc: silence: waiting out %llu ms of backoff before asking again\n",
                       (unsigned long long)((g_retry_after_us - disc_now_us()) / 1000ull));
            }
            fill_silence(dst, n);
            return 0;
        } else {
            /* A genuine new try -- the first since a failure, or a seek past both windows. Worth a wait. */
            g_warm_wait = 1;
            cur = pick_slot(track, start);
            if (start_window(cur, track, start, (uint32_t)want) != 0) return -1;   /* the track ends here */
            bound = cold ? NK_DISC_WAIT_COLD_US : (g_warm_wait ? NK_DISC_WAIT_US : 0ull);
        }

        /* The one wait, and only where waiting can help:
         *   cold -- the ring is empty, so this is a hitch or the start of the music, not both;
         *   warm, ours, and the last one answered -- the ask went out most of a second ago, so give it
         *       the rest of the ring's depth;
         *   warm after a timeout -- the network has just demonstrated it does not answer within 200 ms,
         *       so stop paying for the proof until a fresh try succeeds.
         */
        uint64_t t0 = disc_now_us();
        int r = bound ? nkjs_wait(cur, g_win[cur].tok, (uint32_t)(bound / 1000ull))
                      : nkjs_poll(cur, g_win[cur].tok);
        uint32_t spent = (uint32_t)(disc_now_us() - t0);
        poll_window(cur);
        if (bound) g_wait++;            /* a poll is not a wait: only count the ones that stopped the game */
        if (spent > g_wait_max_us) g_wait_max_us = spent;
        if (spent > 100000u)
            printf("disc: the game waited %u ms for a Range read at %llu of track %u (%s)\n",
                   spent / 1000u, (unsigned long long)start, track, cold ? "cold" : "warm");

        if (r == NKJS_200 && !g_range_unsupported) {
            g_range_unsupported = 1;
            printf("disc: %s answered a Range request with 200 and no Range -- CD-DA stays silent (the "
                   "browser build needs a server that answers HTTP Range)\n", g_base);
            return -1;
        }
        if (covers(&g_win[cur], track, byte_off, n)) {
            g_warm_wait = 1;                             /* bytes: it is worth waiting again sometime */
        } else if (r == NKJS_GO) {
            /* Still on its way. This is the case that used to be the bug: giving up on a slow fetch threw
             * away bytes that landed 50 ms later, and a 4-second outage muted a track for the rest of the
             * session. Silence for this sector, no new ask (the one in flight is the ask), and the next
             * pump picks the window up when it fills. */
            if (disc_now_us() >= g_sil_log_us) {              /* once a second, not once a sector */
                g_sil_log_us = disc_now_us() + 1000000ull;
                printf("disc: silence: a window is still in flight at %llu and this one has no bytes\n",
                       (unsigned long long)g_win[cur].off);
            }
            fill_silence(dst, n);
            return 0;
        } else {
            if (!cold)
                g_warm_wait = 0;
            g_win[cur].st = NK_WIN_EOF; g_win[cur].len = 0;
            g_fetch_fail++;
            g_retry_after_us = disc_now_us() + NK_DISC_BACKOFF_US;
            if (r == NKJS_NET || r == NKJS_NONE)
                printf("disc: a Range read at %llu of track %u got no answer -- silence for %u ms and then "
                       "again (the track is NOT abandoned)\n", (unsigned long long)start, track,
                       (unsigned)(NK_DISC_BACKOFF_US / 1000ull));
            fill_silence(dst, n);
            return 0;
        }
    }
    memcpy(dst, g_win_bytes[cur] + (byte_off - g_win[cur].off), n);
    g_reads++; g_read_bytes += n;

    /* Read ahead once fewer than NK_DISC_LEAD_SECTORS stand between the read pointer and the end of the
     * window -- and do not wait for it. That is the whole of the stutter fix: the ask is fire-and-forget,
     * so the round trip happens while the game is still playing the 0.85 s of samples left in the window. */
    uint64_t next = g_win[cur].off + g_win[cur].len;
    uint64_t limit = (track < NK_DISC_TRACKS) ? g_track_bytes[track] : 0ull;
    if (g_win[cur].st == NK_WIN_READY && !g_win[cur].eof_short &&
        byte_off + n > next - (NK_DISC_FRAME * NK_DISC_LEAD_SECTORS) &&
        (!limit || next < limit)) {            /* nothing past the last byte of the track to read ahead to */
        int other = 1 - cur;
        if (!(g_win[other].track == track && g_win[other].off == next &&
              (g_win[other].st == NK_WIN_FILLING || g_win[other].st == NK_WIN_READY)))
            start_window(other, track, next, NK_DISC_WINDOW_BYTES);
    }
    return 0;
}

void nk_disc_install(const char *base) {
    nk_disc_base(base);
    cd_iso_set_remote(nk_disc_size, nk_disc_pread);
}

void nk_disc_stats(uint32_t *reads, uint32_t *read_bytes, uint32_t *fetches, uint32_t *fetch_bytes,
                   uint32_t *fails, uint32_t *waits, uint32_t *wait_max_us, uint32_t *silence) {
    if (reads) *reads = g_reads;
    if (read_bytes) *read_bytes = g_read_bytes;
    if (fetches) *fetches = g_fetches;
    if (fetch_bytes) *fetch_bytes = g_fetch_bytes;
    if (fails) *fails = g_fetch_fail;
    if (waits) *waits = g_wait;
    if (wait_max_us) *wait_max_us = g_wait_max_us;
    if (silence) *silence = g_silence;
}

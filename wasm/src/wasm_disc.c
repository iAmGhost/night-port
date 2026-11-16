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
 * The window is asked for by URL, not by track number, because a track may have two files: the raw
 * sectors and the encoded version of them (wasm/src/wasm_mp3.c). The same window, the same backoff and
 * the same rules carry both, which is the point of addressing it that way -- the encoded reader gets
 * everything this file learned the hard way for 30 lines of its own.
 * * A track the server does not have is REFUSED, exactly like a missing trackNN.bin: cd_iso marks it
 * absent and denies its reads, so it plays as silence rather than as invented bytes. A server that
 * answers a Range request with `200 OK` and the whole file is treated the same way (said once, in the
 * log): that is a server this build cannot stream from at all, and letting it through would trade a
 * 300 KB read-ahead for an 87 MB download. Note the distinction from everything else -- a 404, a 500,
 * a timeout and a dead socket are one window's worth of nothing, not a verdict on the server.
 */
#include "wasm_disc.h"

#include "cd/cd_iso.h"
#include "wasm_mp3.h"

#include <emscripten.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* 128 raw frames = 301 056 B = 3.42 s of Red Book audio: the largest window this file asks for, and the
 * size of its own buffers. A source that wants less in hand asks for less lead (see disc_pread_url). */
#define NK_DISC_WINDOW       128u
#define NK_DISC_WINDOW_BYTES (NK_DISC_WINDOW * 2352u)
#define NK_DISC_FRAME        2352u
/* Start the next fetch while this many sectors are still buffered behind a raw read pointer. */
#define NK_DISC_LEAD_SECTORS 64u
#define NK_DISC_URL          512
#define NK_DISC_LEAD_RAW     (NK_DISC_FRAME * NK_DISC_LEAD_SECTORS)
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
    char     url[NK_DISC_URL];   /* what the bytes are, and what the two windows are compared on */
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
static uint32_t g_pre_reads;               /* reads answered out of bytes the page downloaded up front */
static uint32_t g_pre_bytes;
static uint32_t g_wait, g_wait_max_us;     /* what a read that had to wait cost the GAME */
static uint64_t g_retry_after_us;          /* the backoff after a window got nothing */
static uint64_t g_sil_log_us;              /* one silence line a second, not one a sector */
static int      g_warm_wait = 1;           /* stop standing still for warm reads once one has timed out */
static int      g_range_unsupported;

/* What a URL is, in bytes, remembered from the HEAD that asked. It is here so the read-ahead can be
 * clamped: a window that starts inside the last stretch of a file asks for bytes the file does not have,
 * and this repo's own server answers that with a closed connection -- see nkjs_start. Measured: every
 * long run died at the last sector of the show's track.
 *
 * Keyed by URL rather than by track number because a track can now have two files, and both may be
 * HEADed in one boot: the raw sectors, and the encoded version of them. An entry with bytes == 0 means
 * "present, length unknown" -- a server that refuses HEAD, which is survivable, but leaves no clamp. */
#define NK_SIZE_CACHE 40u
typedef struct { char url[NK_DISC_URL]; uint64_t bytes; int known; } nk_size;
static nk_size  g_size[NK_SIZE_CACHE];

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

/* A request answered by a file the visitor brought instead of by the network (wasm/runtime/byod.js).
 * `nklocal:<anything>/trackNN.bin` asks for the track the same way `data/cd/trackNN.bin` does -- by last
 * path segment -- and the answer is a slice of a local File: { file, off, len }.  The same slot, the same
 * token, the same "superseded means nobody wants these bytes" rule: what a local file changes is where the
 * bytes come from, not any of the rules about what happens when they do not arrive. */
EM_JS(int, nkjs_start, (const char *url, uint32_t off, uint32_t len, uint8_t *dst, uint32_t slot), {
    const D = globalThis.__nkDisc || (globalThis.__nkDisc = { s: [], tok: 1 });
    let s = D.s[slot];
    if (!s) s = D.s[slot] = { busy: 0, tok: 0, off: 0, len: 0, got: -1, err: 0, p: null };
    /* The same ask is already running: hand back its token rather than asking twice. */
    if (s.busy && s.off === off && s.len === len) return s.tok;
    const tok = D.tok++;
    s.busy = 1; s.tok = tok; s.off = off; s.len = len; s.got = -1; s.err = 0; s.msg = '';
    const myUrl = UTF8ToString(url);
    const local = myUrl.lastIndexOf('/') >= 0 ? myUrl.slice(myUrl.lastIndexOf('/') + 1) : myUrl;
    const e = myUrl.indexOf('nklocal:') === 0 && globalThis.__nkLocal
            ? globalThis.__nkLocal.map.get(local) : null;
    if (e) {
        s.p = e.file.slice(e.off + off, e.off + off + len).arrayBuffer()
            .then((buf) => {
                if (s.tok !== tok) return;                    /* again: superseded mid-read */
                const body = new Uint8Array(buf);
                const n = Math.min(body.length, len);
                HEAPU8.set(body.subarray(0, n), dst);
                s.got = n; s.err = 0; s.busy = 0;
            })
            .catch((ex) => { if (s.tok === tok) { s.err = -3; s.got = -1; s.busy = 0;
                                                  s.msg = 'local: ' + String(ex && ex.message || ex); } });
        return tok;
    }
    s.p = fetch(myUrl, { headers: { 'Range': 'bytes=' + off + '-' + (off + len - 1) } })
        .then(async (r) => {
            if (s.tok !== tok) return;                        /* superseded: these bytes are nobody's */
            if (r.status === 200) { s.err = -5; s.msg = 'HTTP 200, Range ignored'; s.busy = 0; return; }
            if (r.status !== 206) { s.err = -4; s.msg = 'HTTP ' + r.status; s.busy = 0; return; }
            const body = new Uint8Array(await r.arrayBuffer());
            if (s.tok !== tok) return;                        /* and again, after the body */
            const n = Math.min(body.length, len);
            HEAPU8.set(body.subarray(0, n), dst);             /* one synchronous block: C cannot be
                                                               * reading this window while it runs */
            s.got = n; s.err = 0; s.busy = 0;
        })
        /* Keep what the browser said. A phone log that says only "got no answer" cannot tell a dead
         * server (ERR_CONNECTION_REFUSED) from a sleeping radio (TypeError: Failed to fetch) from a
         * bug of ours -- and the three want different fixes. */
        .catch((e) => { if (s.tok === tok) { s.err = -3; s.got = -1; s.busy = 0;
                                            s.msg = String((e && e.name) || 'Error') + ': ' +
                                                      String((e && e.message) || e); } });
    return tok;
});

/* The reason the last ask for this slot failed, as the browser phrased it, for the log. */
EM_JS(void, nkjs_err_msg, (uint32_t slot, char *dst, uint32_t cap), {
    const D = globalThis.__nkDisc || (globalThis.__nkDisc = { s: [], tok: 1 });
    const s = D.s[slot];
    const m = String((s && s.msg) || '').slice(0, (cap >>> 0) - 1);
    const b = new TextEncoder().encode(m);
    HEAPU8.fill(0, dst, dst + cap);
    HEAPU8.set(b, dst);
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

/* ---- bytes the page already has --------------------------------------------------------------------
 * The page can download the whole score before the game starts and hold it in its own memory (docs/463,
 * "preload"). Every read is then answered here, out of a JS Uint8Array -- no socket, no window, no backoff,
 * and no hole. It is deliberately the same seam the network answers through, because that is the point:
 * nothing above it -- the byte-to-sample clock, the FIFO, the game's LBA arithmetic -- learns that the
 * network was ever part of the conversation.
 *
 * The bytes stay on the JS side of the wall: 50 MB of them would have to share the module's heap with 51 MB
 * of NIGHT.RES, and copying a 16 KB chunk per read is microseconds.
 *
 * Only a request wholly inside a preloaded buffer is answered here; anything at or past the tail falls
 * through to the normal path, which knows file sizes and what an EOF means. In practice that is everything:
 * the index gives the exact byte length and the encoder leaves slack past every track's last sample.
 */
EM_JS(int, nkjs_pre_read, (const char *url, uint32_t off, uint8_t *dst, uint32_t n), {
  const P = globalThis.__nkPre;
  if (!P || !P.map) return -1;
  const u = UTF8ToString(url);
  const b = P.map.get(u) || P.map.get(u.split('/').pop());   /* the last segment matches whatever --audio-dir */
  if (!b || off + n > b.length) return -1;
  HEAPU8.set(b.subarray(off, off + n), dst);
  return n >>> 0;
});

/* The length a preloaded file would answer with, or -1 for "the page does not hold this". */
EM_JS(int, nkjs_pre_size, (const char *url), {
  const P = globalThis.__nkPre;
  if (!P || !P.map) return -1;
  const u = UTF8ToString(url);
  const b = P.map.get(u) || P.map.get(u.split('/').pop());
  return b ? (b.length >>> 0) : -1;
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
    const u = UTF8ToString(url);
    if (u.indexOf('nklocal:') === 0) {                       /* the page holds this one: no network to ask */
        const local = u.lastIndexOf('/') >= 0 ? u.slice(u.lastIndexOf('/') + 1) : u;
        const e = globalThis.__nkLocal && globalThis.__nkLocal.map.get(local);
        if (!e) return -1;
        HEAPU32[out32 >> 2] = e.len >>> 0;
        return 0;
    }
    let r;
    try { r = await fetch(u, { method: 'HEAD' }); } catch (e) { return -1; }
    if (!r.ok) return -1;
    const cl = r.headers.get('content-length');
    HEAPU32[out32 >> 2] = cl ? parseInt(cl, 10) : 0;   /* no content-length: present, length unknown */
    return 0;
});

EM_ASYNC_JS(int, nkjs_sleep, (uint32_t ms), { await new Promise((r) => setTimeout(r, ms)); return 0; });

/* One GET, whole, for the small files that are not worth a window: an index, a manifest. dst is the
 * caller's and gets a NUL the caller can strlen. -1 = no answer, which is a normal answer for an
 * optional file. */
EM_ASYNC_JS(int, nkjs_get_text, (const char *url, char *dst, uint32_t cap), {
    let r;
    try { r = await fetch(UTF8ToString(url)); } catch (e) { return -1; }
    if (!r.ok) return -1;
    const body = new Uint8Array(await r.arrayBuffer());
    const n = Math.min(body.length, cap - 1);
    HEAPU8.set(body.subarray(0, n), dst);
    HEAPU8[dst + n] = 0;
    return n;
});

void nk_disc_base(const char *base) { snprintf(g_base, sizeof g_base, "%s", base ? base : ""); }

static void track_url(uint32_t track, const char *ext, char *out, size_t n) {
    snprintf(out, n, "%s/track%02u%s", g_base, track, ext);
}

/* ---- what a URL is, in bytes ---------------------------------------------------------------------
 * Tried three times, because this is the one answer the port cannot take back: cd_iso marks a track it
 * cannot size permanently absent (g_track_absent, cleared only by a re-open), so a HEAD lost to one blip
 * at boot mutes that track for the whole session -- the "and it never came back" symptom with nothing
 * for the player to have done. 200 ms and 800 ms between tries, in the boot window where nobody is
 * playing yet. */
static nk_size *size_slot(const char *url, int create)
{
    for (unsigned i = 0; i < NK_SIZE_CACHE; i++)
        if (g_size[i].known && !strcmp(g_size[i].url, url)) return &g_size[i];
    if (!create) return 0;
    for (unsigned i = 0; i < NK_SIZE_CACHE; i++)
        if (!g_size[i].known) {
            snprintf(g_size[i].url, sizeof g_size[i].url, "%s", url);
            g_size[i].known = 1; g_size[i].bytes = 0;
            return &g_size[i];
        }
    return 0;
}

int nk_disc_size_url(const char *url, uint64_t *out_bytes)
{
    /* A preloaded file's length is the length of the bytes the page is holding: no HEAD to wait for. */
    int pre = nkjs_pre_size(url);
    if (pre > 0) {
        nk_size *e = size_slot(url, 1);
        if (e) e->bytes = *out_bytes = (uint64_t)pre;
        else *out_bytes = (uint64_t)pre;
        return 0;
    }
    const uint32_t backoff_ms[] = { 0, 200, 800 };
    for (unsigned t = 0; t < 3; t++) {
        if (backoff_ms[t]) nkjs_sleep(backoff_ms[t]);
        uint32_t bytes = 0;
        if (nkjs_head_size(url, &bytes) == 0) {
            nk_size *e = size_slot(url, 1);
            if (e) e->bytes = *out_bytes = (uint64_t)bytes;
            else *out_bytes = (uint64_t)bytes;
            if (t) printf("disc: %s answered its size on try %u\n", url, t + 1u);
            return 0;
        }
    }
    printf("disc: %s did not answer a HEAD in three tries\n", url);
    return -1;
}

/* See wasm_disc.h: a length that is already known does not need asking for. */
void nk_disc_size_set(const char *url, uint64_t bytes)
{
    nk_size *e = size_slot(url, 1);
    if (e) e->bytes = bytes;
}

/* cd_iso's question, in the units cd_iso thinks in: "how long is track N, in Red Book bytes". Answered
 * out of the encoded index when the track has encoded bytes, because that is the only claim about the
 * length of the music inside an MP3 -- and the geometry the game sees must not change when the bytes
 * underneath it do. */
int nk_disc_size(uint32_t track, uint64_t *out_bytes)
{
    uint64_t rb = nk_mp3_redbook_bytes(track);
    if (rb) { *out_bytes = rb; return 0; }
    char url[NK_DISC_URL];
    track_url(track, ".bin", url, sizeof url);
    return nk_disc_size_url(url, out_bytes);
}

/* Start a window. 0 = asked (adopt it with poll_window), -1 = there is nothing to ask for, because the
 * offset is past the last byte of the file. */
static int start_window(int slot, const char *url, uint64_t start, uint32_t want) {
    nk_window *w = &g_win[slot];
    nk_size *e = size_slot(url, 0);
    uint64_t limit = e ? e->bytes : 0ull;
    if (limit && start >= limit) {
        snprintf(w->url, sizeof w->url, "%s", url);
        w->off = start; w->want = 0; w->len = 0; w->eof_short = 1; w->st = NK_WIN_EOF;
        return -1;
    }
    if (limit && start + want > limit) want = (uint32_t)(limit - start);
    snprintf(w->url, sizeof w->url, "%s", url);
    w->tok   = (uint32_t)nkjs_start(url, (uint32_t)start, want, g_win_bytes[slot], (uint32_t)slot);
    w->off = start; w->want = want; w->len = 0; w->eof_short = 0; w->st = NK_WIN_FILLING;
    return 0;
}

/* Adopt whatever the browser has done with a filling window. Returns its state. */
static int poll_window(int slot) {
    nk_window *w = &g_win[slot];
    if (w->st != NK_WIN_FILLING) return w->st;
    int r = nkjs_poll(slot, w->tok);
    if (r == NKJS_GO) return NK_WIN_FILLING;
    if (r == NKJS_NONE) { w->st = NK_WIN_EMPTY; w->url[0] = 0; w->len = 0; return NK_WIN_EMPTY; }
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
static int covers(const nk_window *w, const char *url, uint64_t off, uint64_t n) {
    return w->st == NK_WIN_READY && !strcmp(w->url, url) && off >= w->off && off + n <= w->off + w->len;
}

/* Which slot to fill. A window that would cover `start` for this file is worth more than one that is
 * merely full, and a window that is still filling is worth more than one that is finished and useless. */
static int pick_slot(const char *url, uint64_t start) {
    int use[2];
    for (int i = 0; i < 2; i++) {
        const nk_window *w = &g_win[i];
        int would_cover = (!strcmp(w->url, url) && w->st != NK_WIN_EOF && w->off <= start &&
                           start < w->off + (w->st == NK_WIN_READY ? w->len : w->want));
        use[i] = would_cover ? 2 : (w->st == NK_WIN_FILLING ? 1 : 0);
    }
    if (use[0] != use[1]) return use[0] < use[1] ? 0 : 1;
    return (g_win[0].off <= g_win[1].off) ? 0 : 1;
}

/* The answer when there are no bytes: silence, which is what an empty ring plays and what the desktop
 * host plays for a track it cannot open. Counted, because a player hears it. A source that asked to be
 * told (report_fail) gets -1 instead and decides for itself -- an MP3 reader that answered a hole with
 * silence would lose the byte-to-sample clock that its next seek depends on. */
/* The MP3 path answers its own holes (a decode that got no bytes makes its own silence), and those bytes
 * were invisible here: a phone log read `silence=0B` while the song was 400,000 frames of nothing. Same
 * silence, different layer, so the same counter. */
void nk_disc_silence_add(uint32_t bytes) { g_silence += bytes; }

static void fill_silence(void *dst, uint32_t n) {
    memset(dst, 0, n);
    g_reads++; g_read_bytes += n; g_silence += n;
}

/* The byte source, in one place: `n` bytes at `off` of `url`, with `lead` bytes of the file kept
 * buffered ahead of the read pointer before another window is asked for.
 *
 * The window is four times the lead, clamped into this file's buffers, which is how one number per
 * source sets both: the raw sectors ask for 0.85 s of audio and get 301 KB (3.42 s) at a time, and an
 * encoded track asks for 0.9 s of *its* bitrate and gets four times that. Same time ahead of the game,
 * in whatever unit the file is measured in. */
static int disc_pread_url(const char *url, uint64_t byte_off, void *dst, uint32_t n,
                         uint32_t lead, int report_fail) {
    if (n == 0) return 0;
    if (g_range_unsupported) return -1;
    /* Preloaded bytes answer first: a read the page can serve from memory is not a network question at all. */
    if (byte_off + n <= 0xffffffffull) {
        int got = nkjs_pre_read(url, (uint32_t)byte_off, dst, n);
        if (got == (int)n) {
            g_reads++; g_read_bytes += n;
            g_pre_reads++; g_pre_bytes += n;
            return 0;
        }
    }
    if (!lead) lead = NK_DISC_LEAD_RAW;
    uint32_t win = lead * 4u;
    if (win > NK_DISC_WINDOW_BYTES) win = NK_DISC_WINDOW_BYTES;
    if (win < n) win = n;

    for (int i = 0; i < 2; i++) poll_window(i);           /* nothing in flight stays invisible */

    int cur = -1;
    for (int i = 0; i < 2; i++) if (covers(&g_win[i], url, byte_off, n)) cur = i;
    if (cur < 0) {
        uint64_t start = byte_off;
        uint32_t want  = win;
        if (byte_off + n > start + want) want = (uint32_t)(byte_off + n - start);   /* never under-read */

        /* Cold = this file has never been read at all, so nothing is queued to play. The ring is empty
         * whether we wait or not, and waiting is the difference between the music starting on its first
         * sample and losing its first second to silence. */
        int cold = (strcmp(g_win[0].url, url) && strcmp(g_win[1].url, url));

        /* Is a request already on its way that can answer this -- this byte, or one of those behind it?
         * If so it is the ask; do not make another one. Skipping this question is what made a stall
         * expensive instead of merely silent: with the reader walking on one sector a pump and the ring
         * empty, every pump asked for a new window a little further on, evicted the window that was on its
         * way, and threw the answer away when it landed (stale=true, once per response). */
        cur = -1;
        for (int i = 0; i < 2; i++)
            if (g_win[i].st == NK_WIN_FILLING && !strcmp(g_win[i].url, url) && g_win[i].off <= start &&
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
            if (report_fail) { g_reads++; g_read_bytes += n; return -1; }
            fill_silence(dst, n);
            return 0;
        } else {
            /* A genuine new try -- the first since a failure, or a seek past both windows. Worth a wait. */
            g_warm_wait = 1;
            cur = pick_slot(url, start);
            if (start_window(cur, url, start, want) != 0) return -1;   /* the file ends here */
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
            printf("disc: the game waited %u ms for a Range read at %llu of %s (%s)\n",
                   spent / 1000u, (unsigned long long)start, url, cold ? "cold" : "warm");

        if (r == NKJS_200 && !g_range_unsupported) {
            g_range_unsupported = 1;
            printf("disc: %s answered a Range request with 200 and no Range -- CD-DA stays silent (the "
                   "browser build needs a server that answers HTTP Range)\n", url);
            return -1;
        }
        if (covers(&g_win[cur], url, byte_off, n)) {
            g_warm_wait = 1;                             /* bytes: it is worth waiting again sometime */
        } else if (r == NKJS_GO) {
            /* Still on its way. This is the case that used to be the bug: giving up on a slow fetch threw
             * away bytes that landed 50 ms later, and a 4-second outage muted a track for the rest of the
             * session. Silence for this read, no new ask (the one in flight is the ask), and the next call
             * picks the window up when it fills. */
            if (disc_now_us() >= g_sil_log_us) {              /* once a second, not once a sector */
                g_sil_log_us = disc_now_us() + 1000000ull;
                printf("disc: silence: a window is still in flight at %llu and this one has no bytes\n",
                       (unsigned long long)g_win[cur].off);
            }
            if (report_fail) { g_reads++; g_read_bytes += n; return -1; }
            fill_silence(dst, n);
            return 0;
        } else {
            if (!cold)
                g_warm_wait = 0;
            g_win[cur].st = NK_WIN_EOF; g_win[cur].len = 0;
            g_fetch_fail++;
            g_retry_after_us = disc_now_us() + NK_DISC_BACKOFF_US;
            if (r == NKJS_NET || r == NKJS_NONE) {
                char msg[96];
                nkjs_err_msg(cur, msg, sizeof msg);            /* the browser's own words, not our guess */
                printf("disc: a Range read at %llu of %s got no answer -- silence for %u ms and then "
                       "again (the track is NOT abandoned)%s%s%s\n", (unsigned long long)start, url,
                       (unsigned)(NK_DISC_BACKOFF_US / 1000ull),
                       msg[0] ? " (" : "", msg, msg[0] ? ")" : "");
            }
            if (report_fail) { g_reads++; g_read_bytes += n; return -1; }
            fill_silence(dst, n);
            return 0;
        }
    }
    memcpy(dst, g_win_bytes[cur] + (byte_off - g_win[cur].off), n);
    g_reads++; g_read_bytes += n;

    /* Read ahead once fewer than `lead` bytes stand between the read pointer and the end of the window --
     * and do not wait for it. That is the whole of the stutter fix: the ask is fire-and-forget, so the
     * round trip happens while the game is still playing the samples left in the window. */
    uint64_t next = g_win[cur].off + g_win[cur].len;
    nk_size *e = size_slot(url, 0);
    uint64_t limit = e ? e->bytes : 0ull;
    if (g_win[cur].st == NK_WIN_READY && !g_win[cur].eof_short &&
        byte_off + n > next - lead &&
        (!limit || next < limit)) {            /* nothing past the last byte of the file to read ahead to */
        int other = 1 - cur;
        if (!(g_win[other].st == NK_WIN_FILLING || g_win[other].st == NK_WIN_READY) ||
            !(!strcmp(g_win[other].url, url) && g_win[other].off == next))
            start_window(other, url, next, win);
    }
    return 0;
}

int nk_disc_read(const char *url, uint64_t off, void *dst, uint32_t n, uint32_t lead, int report_fail)
{
    return disc_pread_url(url, off, dst, n, lead, report_fail);
}

/* One GET for the small files a window would be silly for. */
int nk_disc_get_text(const char *url, char *dst, uint32_t cap) { return nkjs_get_text(url, dst, cap); }

/* cd_iso's read callback: one 2352-byte sector of track N. Either the encoded version of that sector
 * (wasm/src/wasm_mp3.c, if the audio index says the track has one), or the sector itself out of the
 * track file, answered out of the window this file keeps warm. */
/* Silence the music without silencing the disc. The page's "download the music" box off means the game runs
 * and plays -- with the soundtrack as silence and not one byte of it fetched. That is not what `--no-audio`
 * does: refusing the MP3 path falls back to streaming raw sectors, which is 176 KB/s instead of 16, i.e.
 * *more* bytes, which is the exact opposite of the reason the box exists. Track 1 is the data track and is
 * left alone: this is about music, not about the disc. */
static int g_silent_audio;

void nk_disc_silent_audio(int on) { g_silent_audio = on ? 1 : 0; }

int nk_disc_pread(uint32_t track, uint64_t byte_off, void *dst, uint32_t n) {
    if (g_silent_audio && track != 1) { fill_silence(dst, n); return 0; }
    if (nk_mp3_has(track)) return nk_mp3_pread(track, byte_off, dst, n);
    char url[NK_DISC_URL];
    track_url(track, ".bin", url, sizeof url);
    return disc_pread_url(url, byte_off, dst, n, NK_DISC_LEAD_RAW, 0);
}

void nk_disc_install(const char *base) {
    nk_disc_base(base);
    cd_iso_set_remote(nk_disc_size, nk_disc_pread);
}

void nk_disc_pre_stats(uint32_t *reads, uint32_t *bytes) {
    if (reads) *reads = g_pre_reads;
    if (bytes) *bytes = g_pre_bytes;
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

/* wasm_disc.c -- the browser host's disc byte source: HTTP Range reads behind the cd_iso.h remote
 * seam.  docs/457.
 *
 * WHAT THIS IS
 * ------------
 * The desktop host reads CD-DA from extracted/cd/trackNN.bin with fopen/fseek/fread; the Pocket host
 * reads the same sectors from APF data slots (pocket/src/night/of_disc.c). A browser has neither, and
 * it must not download 604 MB of tracks to hear the one song a scene plays. So this file answers
 * cd_iso.c's two remote callbacks -- "how long is track N", "give me 2352 bytes at an offset" -- with
 * Range requests, and keeps a sliding read-ahead window so the pump asks the network something like
 * once every 1.7 s of music instead of 75 times a second.
 *
 * The callbacks run inside the audio pump, which runs inside the game's own busy-wait, so a fetch is a
 * yield: with -sASYNCIFY the game's stack unwinds, the browser gets its event loop back, and the read
 * resumes in place. That is only tolerable because of the window -- one round trip per 1.7 s, taken
 * while 0.4 s of audio is still buffered.
 *
 * A track the server does not have is REFUSED, exactly like a missing trackNN.bin: cd_iso marks it
 * absent and denies its reads, so it plays as silence rather than as invented bytes. A server that will
 * not answer a Range request is treated the same way (said once, in the log), because a 200 OK carrying
 * the whole file would turn a 150 KB read-ahead into an 87 MB download.
 */
#include "wasm_disc.h"

#include "cd/cd_iso.h"

#include <emscripten.h>
#include <stdio.h>
#include <string.h>

/* 64 raw frames = 150 528 B = 1.71 s of Red Book audio per request. */
#define NK_DISC_WINDOW      64u
#define NK_DISC_WINDOW_BYTES (NK_DISC_WINDOW * 2352u)
#define NK_DISC_FRAME        2352u
/* Start the next fetch while this many sectors are still buffered behind the read pointer. */
#define NK_DISC_LEAD_SECTORS 16u

typedef struct {
    uint32_t track;          /* 0 = empty */
    uint64_t off;            /* byte offset of the window's first byte */
    uint32_t len;            /* bytes held */
    int      eof;            /* the fetch that filled this window came back short or failed */
    uint8_t  bytes[NK_DISC_WINDOW_BYTES];
} nk_window;

/* Two windows, so refilling the next one never evicts the one being read. */
static nk_window g_win[2];

static char     g_base[512]; /* ".../data/cd" -- track files are <base>/trackNN.bin */
static uint32_t g_reads, g_read_bytes, g_fetches, g_fetch_bytes, g_fetch_fail;
static int      g_range_unsupported;

/* The byte length of each track, remembered from the HEAD cd_iso asks for when it opens the track. It is
 * here so the read-ahead can be clamped: a window that starts inside the last 64 sectors of a track asks
 * for bytes the file does not have, and this repo's own server answers that with a closed connection --
 * see nk_js_fetch_range. Measured: every long run died at the last sector of the show's track. */
#define NK_DISC_TRACKS 64u
static uint64_t g_track_bytes[NK_DISC_TRACKS];

/* One Range read. Returns the bytes copied, -1 when the server will not answer Range (or has no such
 * file), or -2 when the request itself never got an answer -- a network error, or a server that closed
 * the connection without a response, which is what this repo's own server does to a Range that starts
 * past the end of the file (net::ERR_EMPTY_RESPONSE). The distinction matters: the first is a server that
 * cannot serve this build at all, the second is one sector's worth of nothing, and treating the second
 * like the first would mute the rest of the run. Without the try/catch the rejected promise unwinds
 * through the game's own stack -- measured: frames stop, the rAF gap reaches 20 s, the run is over.
 *
 * The await inside EM_ASYNC_JS is what makes this a yield rather than a blocking XHR that would freeze
 * the tab. */
EM_ASYNC_JS(int, nk_js_fetch_range, (const char *url, uint32_t off, uint32_t len, uint8_t *dst), {
    let r;
    try {
        r = await fetch(UTF8ToString(url), { headers: { 'Range': 'bytes=' + off + '-' + (off + len - 1) } });
    } catch (e) { return -2; }
    if (r.status !== 206) return -1;               /* no Range support, or no such file */
    const body = new Uint8Array(await r.arrayBuffer());
    const n = Math.min(body.length, len);
    HEAPU8.set(body.subarray(0, n), dst);
    return n;
});

/* HEAD for the file's length. 0 = answered (0 in *out32 means "present, length unknown"), -1 = not there. */
EM_ASYNC_JS(int, nk_js_head_size, (const char *url, uint32_t *out32), {
    let r;
    try { r = await fetch(UTF8ToString(url), { method: 'HEAD' }); } catch (e) { return -1; }
    if (!r.ok) return -1;
    const cl = r.headers.get('content-length');
    HEAPU32[out32 >> 2] = cl ? parseInt(cl, 10) : 0;   /* no content-length: present, length unknown */
    return 0;
});

void nk_disc_base(const char *base) { snprintf(g_base, sizeof g_base, "%s", base ? base : ""); }

static void track_url(uint32_t track, char *out, size_t n) {
    snprintf(out, n, "%s/track%02u.bin", g_base, track);
}

int nk_disc_size(uint32_t track, uint64_t *out_bytes) {
    char url[600];
    track_url(track, url, sizeof url);
    uint32_t bytes = 0;
    if (nk_js_head_size(url, &bytes) != 0) return -1;
    *out_bytes = (uint64_t)bytes;
    if (track < NK_DISC_TRACKS) g_track_bytes[track] = *out_bytes;   /* 0 = the HEAD did not carry it */
    return 0;
}

static int covers(const nk_window *w, uint32_t track, uint64_t off, uint32_t n) {
    return w->track == track && off >= w->off && off + n <= w->off + w->len;
}

/* Fetch a window for `track` starting at `start`. 0 = the window now holds bytes. -1 = the server does not
 * answer Range (or has no such file), which is fatal for CD-DA as a whole. 1 = nothing came back for this
 * window: past the end of the track, or a network error -- the window is empty and marked eof, which is
 * what the caller already treats as "the bytes stop here". */
static int fetch_window(int slot, uint32_t track, uint64_t start, uint32_t want) {
    uint64_t limit = (track < NK_DISC_TRACKS) ? g_track_bytes[track] : 0ull;
    if (limit && start >= limit) {                     /* nothing to ask for */
        nk_window *w = &g_win[slot];
        w->track = track; w->off = start; w->len = 0; w->eof = 1;
        return 1;
    }
    if (limit && start + want > limit) want = (uint32_t)(limit - start);
    char url[600];
    track_url(track, url, sizeof url);
    int got = nk_js_fetch_range(url, (uint32_t)start, want, g_win[slot].bytes);
    nk_window *w = &g_win[slot];
    w->track = track; w->off = start;
    w->len = (got > 0) ? (uint32_t)got : 0;
    w->eof = (got <= 0) || (w->len < want);
    if (got > 0) { g_fetches++; g_fetch_bytes += w->len; return 0; }
    g_fetch_fail++;
    if (got == -2) {                                   /* this read got nothing; CD-DA is fine otherwise */
        printf("disc: a Range read at %llu of track %u got no answer ("   /* docs/459: past EOF, or a stall */
               "the window is treated as the end of the track)\n",
               (unsigned long long)start, track);
        return 1;
    }
    return -1;
}

/* Which slot to fill: one that does not hold this track, and among those the one holding the least
 * useful (oldest-offset) data. A window of another track is always expendable. */
static int pick_slot(uint32_t track, uint64_t start) {
    int a = (g_win[0].track == track && covers(&g_win[0], track, start, 1)) ? 1 : 0;
    int b = (g_win[1].track == track && covers(&g_win[1], track, start, 1)) ? 0 : 1;
    if (g_win[a].track != track && g_win[b].track != track) return (g_win[a].off <= g_win[b].off) ? a : b;
    return (g_win[a].track != track) ? a : b;
}

int nk_disc_pread(uint32_t track, uint64_t byte_off, void *dst, uint32_t n) {
    if (n == 0) return 0;
    if (g_range_unsupported) return -1;

    int cur = -1;
    for (int i = 0; i < 2; i++) if (covers(&g_win[i], track, byte_off, n)) cur = i;
    if (cur < 0) {
        uint64_t start = (byte_off / NK_DISC_FRAME) * NK_DISC_FRAME;
        uint64_t want  = NK_DISC_WINDOW_BYTES;
        if (byte_off + n > start + want) want = byte_off + n - start;   /* never under-read the request */
        cur = pick_slot(track, start);
        int rc = fetch_window(cur, track, start, (uint32_t)want);
        if (rc != 0) {
            if (rc < 0 && !g_range_unsupported) {
                g_range_unsupported = 1;
                printf("disc: %s did not answer a Range request -- CD-DA stays silent (the browser build "
                       "needs a server that answers HTTP Range)\n", g_base);
            }
            return -1;
        }
    }
    memcpy(dst, g_win[cur].bytes + (byte_off - g_win[cur].off), n);
    g_reads++; g_read_bytes += n;

    /* Read ahead once fewer than NK_DISC_LEAD_SECTORS stand between the read pointer and the end of the
     * window, so the network is asked while there is still 0.43 s of samples to play. */
    uint64_t next = g_win[cur].off + g_win[cur].len;
    uint64_t limit = (track < NK_DISC_TRACKS) ? g_track_bytes[track] : 0ull;
    if (!g_win[cur].eof && byte_off + n > next - (NK_DISC_FRAME * NK_DISC_LEAD_SECTORS) &&
        (!limit || next < limit)) {            /* nothing past the last byte of the track to read ahead to */
        int other = 1 - cur;
        if (!(g_win[other].track == track && g_win[other].off == next))
            fetch_window(other, track, next, NK_DISC_WINDOW_BYTES);   /* failure is not fatal here */
    }
    return 0;
}

void nk_disc_install(const char *base) {
    nk_disc_base(base);
    cd_iso_set_remote(nk_disc_size, nk_disc_pread);
}

void nk_disc_stats(uint32_t *reads, uint32_t *read_bytes, uint32_t *fetches, uint32_t *fetch_bytes,
                   uint32_t *fails) {
    if (reads) *reads = g_reads;
    if (read_bytes) *read_bytes = g_read_bytes;
    if (fetches) *fetches = g_fetches;
    if (fetch_bytes) *fetch_bytes = g_fetch_bytes;
    if (fails) *fails = g_fetch_fail;
}

/* wasm_mp3.c -- CD-DA for the browser out of compressed bytes instead of Red Book sectors.  docs/463.
 *
 * WHAT THIS IS
 * ------------
 * The disc the game reads is 604 MB of 44.1 kHz stereo PCM, and a browser should not have to carry that
 * to hear the four minutes a room plays. So a CD-DA read stops being "give me the 2352 bytes at offset X
 * of trackNN.bin" and becomes "give me the 588 samples at sample X", answered out of an MP3 by
 * wasm/src/mp3_stream.c, whose bytes arrive through the same Range-window service that carried the raw
 * sectors (wasm/src/wasm_disc.c).
 *
 * Everything above this file is unchanged on purpose. cd_iso still owns the geometry, the game still
 * asks for LBAs, the pump still paces the ring against its own clock, the audit still sees the byte
 * range the game asked for, and the rate marks still say what rate the samples are at. The one thing
 * that differs is that hearing a track costs its MP3 instead of its sectors -- at 128 kbps, 16 KB/s
 * instead of 176 KB/s.
 *
 * WHY MP3, IN ONE PARAGRAPH: it is the format whose byte-to-sample map needs nothing. Constant bitrate
 * means the clock first_byte + sample * bytes_per_sec / rate puts any sample within a byte of its frame,
 * so a Range window can start decoding anywhere and the game's LBA arithmetic survives intact. Formats
 * with better density per second (Opus, AAC) do not carry that map, and their browsers-only decoders want
 * the whole file at once -- which trades a Range window for a 40 MB peak in a phone browser's heap.
 *
 * THE INDEX, AND WHY IT IS NOT OPTIONAL: the encoded byte length says nothing about how much music is in
 * the file, and cd_iso needs the Red Book length to bound a track (it refuses reads past a track's last
 * frame, and for the last track on the disc there is no following INDEX to bound it with). tracks.tsv,
 * written by wasm/scripts/encode_audio.py next to the files it made, states the Red Book length, the
 * bitrate, the rate, the channel count and the decoded sample count per track. Without it this file does
 * not engage at all, and the port streams raw sectors exactly as it did before.
 *
 * The head padding is what the last column carries: an encoder pads the front of a track with the state
 * its filterbank needs, and a port that ignored the padding would start every song tens of milliseconds
 * late against the picture. wasm/scripts/encode_audio.py measures it -- it decodes each track with the
 * same code the port ships and slides the result across the raw sectors it came from until they line up --
 * and writes the number into the index. 0 is a legal value and means nothing is dropped.
 */
#include "wasm_mp3.h"

#include "mp3_stream.h"
#include "wasm_disc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NK_MP3_TRACKS   64u
#define NK_MP3_URL      512
#define NK_MP3_FIFO   4096u                        /* stereo frames; 1152 are produced per decode */
#define NK_MP3_CHUNK 16384u                        /* bitstream bytes in hand */
/* How many bytes of a track's MP3 to keep buffered ahead of the decode cursor.
 *
 * It used to be 0.9 s of audio, to match the raw path's 64-sector read-ahead, and that was the wrong unit
 * to think in. The raw path pays ~nothing to sit on bytes it has, while this one decodes on the game's own
 * thread and MSCDEX blocks on the call: a window that has not arrived yet is not "a late fetch", it is a
 * stall inside a Red Book read, in the middle of the frame the game is composing. Measured, 0.9 s of lead
 * left 15 awaited reads in a 40 s run, and six of the 2880 sector reads cost the game 2 ms or more.
 * Three seconds of music costs 48 KB of buffering at 128 kbps -- one fortieth of what one raw track costs
 * -- and makes the wait rare enough that the counters below stop moving. */
#define NK_MP3_LEAD_MS  3000u
/* A window is 4 s of audio at the bitrate the index says, clamped into the Range service's own buffer. */
#define NK_MP3_WIN_MIN  32768u

typedef struct {
    uint32_t track;
    uint64_t mp3_bytes;
    uint32_t sectors;                    /* Red Book frames this track is, per the index */
    uint32_t bitrate_kbps, rate, channels;
    uint64_t decoded_samples;            /* what a decoder gets out of the file, tags aside */
    uint64_t delay;                      /* samples the encoder put in front of the music, measured */
    int      present;
} nk_mp3_toc;

static nk_mp3_toc g_toc[NK_MP3_TRACKS];
static int      g_index_loaded, g_index_tracks, g_on;
static char     g_base[256] = "data/audio";

static nk_mp3s  g_s;
static int16_t  g_fifo[NK_MP3_FIFO * 2];
static uint8_t  g_chunk[NK_MP3_CHUNK];
static char     g_url[NK_MP3_URL];
static uint32_t g_track;                 /* 0 = nothing decoded yet */
static uint32_t g_decode_us_n;           /* what the decoder cost the game, in wall microseconds */
static uint64_t g_holes;                 /* frames asked for with no bytes to anchor the clock on */
/* The whole sector read, as MSCDEX waits for it -- not just the decode. A seek can decode half a dozen
 * frames and fetch a window that is not in hand, and it sits on the same critical path as the fill, so a
 * figure that measures only the fill says a stutter is not here when it is. */
static uint32_t g_pread_calls, g_pread_us, g_pread_us_max;
static uint32_t g_seek_calls, g_seek_us, g_seek_us_max;
static uint32_t g_pread_slow2, g_pread_slow8;   /* reads over 2 ms and over 8 ms: a frame at 60 Hz has 16.7 ms
                                                  * to live in, and a stutter is one of these landing in it */
static uint32_t g_track_reads, g_slow_start, g_slow_hot;   /* which of those are the first reads of a track --
                                                            * cold decoder, cold window -- and which are steady
                                                            * state, which is the only kind that hurts */

static uint64_t mp3_now_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
    return 0;
}

static void track_url(uint32_t track, char *out, size_t n) {
    snprintf(out, n, "%s/track%02u.mp3", g_base, track);
}

/* ---- the index ------------------------------------------------------------------------------------ */

/* One line per track: `track mp3_bytes redbook_sectors bitrate_kbps rate channels decoded_samples
 * head_delay_samples`. '#' starts a comment, which is where the encoder writes what it did. strtoull
 * rather than sscanf with SCNu64: this runs in a browser build, and the portable width macros have
 * opinions there. The last column is optional -- a build that measured no delay says 0 and costs nothing. */
static void index_line(char *line) {
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '#' || *line == '\0' || *line == '\n') return;
    uint64_t v[8];
    int n = 0;
    char *p = line;
    while (n < 8) {
        char *end;
        v[n] = strtoull(p, &end, 10);
        if (end == p) break;
        p = end;
        while (*p == ' ' || *p == '\t') p++;
        n++;
    }
    if (n < 7) return;
    uint32_t track = (uint32_t)v[0];
    if (track == 0 || track >= NK_MP3_TRACKS) return;
    nk_mp3_toc *e = &g_toc[track];
    e->track = track;
    e->mp3_bytes = v[1];
    e->sectors = (uint32_t)v[2];
    e->bitrate_kbps = (uint32_t)v[3];
    e->rate = (uint32_t)v[4];
    e->channels = (uint32_t)v[5];
    e->decoded_samples = v[6];
    e->delay = (n >= 8) ? v[7] : 0ull;
    e->present = e->sectors && e->bitrate_kbps && e->rate;
    if (e->present) g_index_tracks++;
}

static int load_index(void)
{
    char url[300];
    static char text[8192];
    if (g_index_loaded) return g_index_tracks;
    g_index_loaded = 1;
    if (!g_on || !g_base[0]) return 0;
    snprintf(url, sizeof url, "%s/tracks.tsv", g_base);
    int n = nk_disc_get_text(url, text, sizeof text);
    if (n <= 0) {
        printf("audio: %s did not answer -- CD-DA will stream raw Red Book sectors\n", url);
        return 0;
    }
    text[sizeof text - 1] = '\0';
    for (char *p = text, *line; p && *p; p = line ? line + 1 : 0) {
        line = strchr(p, '\n');
        if (line) *line = '\0';
        index_line(p);
    }
    if (!g_index_tracks) printf("audio: %s holds no playable track -- raw sectors it is\n", url);
    else printf("audio: %s: %u lossy tracks (%s %.0f kbps %u Hz %u ch), CD-DA will come from there\n",
                url, g_index_tracks,
                g_toc[2].present ? "track02 is" : "first track is",
                (double)(g_toc[2].present ? g_toc[2].bitrate_kbps : 0),
                g_toc[2].present ? g_toc[2].rate : 0,
                g_toc[2].present ? g_toc[2].channels : 0);
    return g_index_tracks;
}

/* ---- the byte source the bitstream reads through ---------------------------------------------------- */

static int g_bytes_ok;   /* did the last ask of the window service get an answer? A hole makes the stream's
                          * position untrusted, and the cure is a re-anchor off the byte clock -- but asking
                          * for that anchor while the link is dead starts a cold Range read per sector (75 a
                          * second, each an Asyncify suspension), and that wedged the page outright. Re-anchor
                          * when the bytes are back; silence is the whole job while they are not. */

static int mp3_read(void *user, uint64_t off, void *dst, uint32_t n)
{
    (void)user;
    nk_mp3_toc *e = (g_track < NK_MP3_TRACKS) ? &g_toc[g_track] : 0;
    uint32_t bps = e ? e->bitrate_kbps * 125u : 16000u;           /* the index's clock, not the stream's */
    uint32_t lead = bps * NK_MP3_LEAD_MS / 1000u;
    int r = nk_disc_read(g_url, off, dst, n, lead, 1);             /* 1 = report failure, do not paper over it */
    g_bytes_ok = (r == 0);
    return r;
}

int nk_mp3_has(uint32_t track)
{
    return (track < NK_MP3_TRACKS) ? g_toc[track].present : 0;
}

uint64_t nk_mp3_redbook_bytes(uint32_t track)
{
    if (!load_index()) return 0;
    if (track >= NK_MP3_TRACKS || !g_toc[track].present) return 0;
    return (uint64_t)g_toc[track].sectors * 2352ull;
}

/* Samples the encoder put in front of the music, from the index. Clamped to what a measurement window
 * could have found: an index that disagrees with itself should cost a little sync, not a song. */
static uint64_t mp3_delay(const nk_mp3_toc *e)
{
    return (e->delay > 6144ull) ? 0ull : e->delay;
}

uint32_t nk_mp3_rate(void)
{
    if (!g_track || g_track >= NK_MP3_TRACKS) return 0;
    return g_toc[g_track].rate;
}

/* Load on demand: the host asks this to decide what to print, and the answer is only worth having once
 * the index has actually been read. In the port the index is already in hand by the time anyone asks --
 * cd_iso sizes a track before anything else does -- but a host that asks first should still get the truth.
 * */
int nk_mp3_ready(void) { return load_index(); }
const char *nk_mp3_source(void) { return g_on ? g_base : ""; }

int nk_mp3_pread(uint32_t track, uint64_t off, void *dst, uint32_t n)
{
    if (!load_index() || track >= NK_MP3_TRACKS || !g_toc[track].present) return -1;
    if (n % 4u) return -1;
    nk_mp3_toc *e = &g_toc[track];
    uint64_t want = off / 4ull;                          /* sample index inside the track */
    uint32_t frames = n / 4u;
    uint64_t total = (uint64_t)e->sectors * 588ull;
    if (want >= total) return -1;                        /* past this track's last sample */
    if (want + frames > total) frames = (uint32_t)(total - want);

    if (g_track != track) {
        g_track = track;
        track_url(track, g_url, sizeof g_url);
        nk_disc_size_set(g_url, e->mp3_bytes);       /* the index knows the length; do not HEAD for it */
        if (nk_mp3s_init(&g_s, mp3_read, 0, g_fifo, NK_MP3_FIFO, g_chunk, sizeof g_chunk) != 0) return -1;
        nk_mp3s_set_delay(&g_s, mp3_delay(e));       /* before the first seek: it is part of the timeline */
        g_track_reads = 0;
        printf("audio: track %u from %s (%.1f MB, %u sectors of Red Book in it, %llu samples of encoder "
               "padding thrown away)\n", track, g_url, (double)e->mp3_bytes / 1048576.0, e->sectors,
               (unsigned long long)g_s.delay);
    }
    /* Ask for the seek when the position really moved -- OR when the stream says its position is a guess.
     * The second half is not decoration: a hole hands the caller silence and moves the stream's clock by
     * exactly that much, so a sector-at-a-time reader sees pos == want forever after and never asks for the
     * re-anchor that would put it back on the byte clock. It plays on, from the wrong bytes. Both sides
     * speak raw samples -- the stream hides its padding inside itself. */
    uint64_t t_all = mp3_now_us();
    if (nk_mp3s_pos(&g_s) != want) {
        uint64_t t_seek = mp3_now_us();
        int r = nk_mp3s_seek(&g_s, want);
        uint32_t du = (uint32_t)(mp3_now_us() - t_seek);
        g_seek_calls++; g_seek_us += du;
        if (du > g_seek_us_max) g_seek_us_max = du;
        if (r != 0) {
            /* No bytes to anchor on. Silence for this read rather than -1, which the host reads as "the
             * track is over" and would abandon the song for a network that blinked -- the exact failure
             * docs/460 is written about. The position stays wrong, so the next sector tries again. */
            memset(dst, 0, n);
            g_holes += frames;
            return 0;
        }
    }

    uint64_t t0 = mp3_now_us();
    uint32_t got = nk_mp3s_fill(&g_s, (int16_t *)dst, frames);
    g_decode_us_n += (uint32_t)(mp3_now_us() - t0);
    uint32_t all = (uint32_t)(mp3_now_us() - t_all);
    g_pread_calls++; g_pread_us += all;
    if (all > g_pread_us_max) g_pread_us_max = all;
    g_track_reads++;
    if (all > 2000ull) { g_pread_slow2++; if (g_track_reads <= 16u) g_slow_start++; else g_slow_hot++; }
    if (all > 8000ull) g_pread_slow8++;
    /* The one answer that must never be given inside a track is a refusal. main_wasm.c's cdda_pump reads -1
     * as the end of the song -- "cdda: read failed ... the track is abandoned", g_cdda_sectors = 0 -- and
     * nothing in the port re-PLAYs a track; only the game's next one does. Its comment says the byte source
     * answers a missing window with silence, which is what the raw path does and what this has to match.
     * Only the check at the top of this function -- `want` past the track's last sample -- returns -1. */
    if (!got) {            /* nothing came of the fill: that is a hole, and a hole is silence. Refusing is
                            * what told the host the song had ended -- see the comment above this line. */
        memset(dst, 0, n);
        g_holes += frames;
        nk_disc_silence_add(frames * 4u);        /* stereo 16-bit: so `silence=` in the pump line counts the
                                                  * holes this layer made, which is where they are made */
        return 0;
    }
    return 0;
}

void nk_mp3_stats(uint64_t *frames, uint64_t *bytes, uint64_t *seeks, uint64_t *silence,
                  uint64_t *map_err_max, uint64_t *decode_us)
{
    if (frames) *frames = g_s.frames;
    if (bytes) *bytes = g_s.bytes_read;
    if (seeks) *seeks = g_s.seeks;
    if (silence) *silence = g_s.silence_frames + g_holes;
    if (map_err_max) *map_err_max = g_s.map_err_max;
    if (decode_us) *decode_us = g_decode_us_n;
}

/* What a sector read cost the game end to end, and the worst single one. The average is the bandwidth
 * question; the maximum is the stutter question, because a frame rate is decided by the worst read that
 * lands in a frame, not by the mean. */
void nk_mp3_pread_stats(uint32_t *calls, uint32_t *us_total, uint32_t *us_max,
                        uint32_t *seek_calls, uint32_t *seek_us_total, uint32_t *seek_us_max,
                        uint32_t *over_2ms, uint32_t *over_8ms)
{
    if (calls) *calls = g_pread_calls;
    if (us_total) *us_total = g_pread_us;
    if (us_max) *us_max = g_pread_us_max;
    if (seek_calls) *seek_calls = g_seek_calls;
    if (seek_us_total) *seek_us_total = g_seek_us;
    if (seek_us_max) *seek_us_max = g_seek_us_max;
    if (over_2ms) *over_2ms = g_pread_slow2;
    if (over_8ms) *over_8ms = g_pread_slow8;
    if (over_2ms) { }   /* the split below is what the pump line prints with them */
}

/* The cold/hot split of the slow reads: a track's first reads pay for a decoder that has seen nothing and a
 * window that has not arrived, and the game is usually mid-transition when it happens. A slow read in the
 * middle of a room is the one that shows up as a hitch. */
void nk_mp3_slow_split(uint32_t *track_start, uint32_t *steady)
{
    if (track_start) *track_start = g_slow_start;
    if (steady) *steady = g_slow_hot;
}

void nk_mp3_install(const char *base, int on)
{
    g_on = on;
    if (base && *base) snprintf(g_base, sizeof g_base, "%s", base);
    if (!on) return;
    memset(g_toc, 0, sizeof g_toc);
    g_index_loaded = 0; g_index_tracks = 0;
    /* The decoder goes with the index it was decoded against. Leaving it live meant a caller that
     * reinstalled -- which is what a test does to get a clean run -- kept the previous pass's position and
     * its byte cursor, so "read this track from sector N" quietly read the tail of the last pass instead. */
    g_track = 0;
}

/* mp3_stream.c -- see mp3_stream.h.  docs/463.
 *
 * This file deliberately knows nothing about where its bytes come from or who listens to them. It knows:
 * a CBR MP3 is a clock, a frame header is found by scanning, a frame is 1152 samples (576 for MPEG-2 and
 * MPEG-2.5 Layer III), a tag frame is skipped and not played, and a byte that does not arrive becomes
 * silence rather than a stalled caller -- the same rule wasm/src/wasm_disc.c was written around.
 *
 * THE STATE, IN ONE SENTENCE: `frame_sample` is the sample number of the MP3 frame sitting at the byte
 * cursor `byte_pos`, and the FIFO holds the samples of frames before that one which have been decoded but
 * not yet handed over -- so the next sample to come out is always frame_sample - fifo_n.
 */
#include "mp3_stream.h"

/* This file owns the vendored decoder's implementation: one translation unit, MINIMP3_IMPLEMENTATION
 * before the include, and only the Layer III path (MINIMP3_ONLY_MP3), because a Red Book track encoded to
 * MP3 is Layer III and everything else is code that cannot be reached. */
#define MINIMP3_IMPLEMENTATION 1
#define MINIMP3_ONLY_MP3 1
#include "minimp3.h"

#include <stdlib.h>
#include <string.h>
#ifdef NK_MP3_TRACE
#include <stdio.h>                  /* the trace is a debugging build of the probe, never the port */
#endif

/* A frame's worth of samples plus the largest frame the spec allows (minimp3's MAX_FREE_FORMAT_FRAME_SIZE
 * is 2304 B, and free-format frames push past that), so a decode never runs off the end of the buffer it
 * was handed. That matters: mp3dec_decode_frame rejects a frame unless the header of the frame AFTER it
 * is inside the buffer, which is why the cursor always keeps 1600+ bytes of slack in front of it. */
#define NK_MP3S_CHUNK_MIN   2048u
#define NK_MP3S_REFILL      1600u
#define NK_MP3S_SAMPLES     1152u
/* Samples of silence per failed byte read. Small on purpose: it is also how quickly the stream notices
 * the bytes have come back. */
#define NK_MP3S_SILENCE      576u
/* How far to back off from an analytic byte estimate before scanning for a header, in frames. The clock
 * is good to about a byte (measured -- see map_err_max and analysis/probes/nk_mp3_probe.c), so this is
 * headroom for a header the decoder rejects, not for arithmetic. */
#define NK_MP3S_BACKOFF        4u
/* Bytes scanned with no frame in them before the stream calls it a hole. */
#define NK_MP3S_SCAN_MAX    32768u

/* A frame is a tag and not music when its payload carries one of these. Only looked for while anchoring:
 * mid-bitstream, those four bytes are somebody's Huffman code. */
static const char g_tags[][5] = { "Xing", "Info", "CBR", "LAME" };

static int has4(const uint8_t *p, uint32_t n, const char *s)
{
    if (n < 4u) return 0;
    for (uint32_t i = 0; i + 4u <= n; i++)
        if (p[i] == (uint8_t)s[0] && p[i + 1] == (uint8_t)s[1] &&
            p[i + 2] == (uint8_t)s[2] && p[i + 3] == (uint8_t)s[3]) return 1;
    return 0;
}

struct nk_mp3s_dec { mp3dec_t d; };

/* ---- the clock ----------------------------------------------------------------------------------- */

static uint64_t nk_mp3s_bpf(const nk_mp3s *s)                       /* bytes per frame */
{ return s->bytes_per_sec ? (s->bytes_per_sec * NK_MP3S_SAMPLES) / s->rate : 0ull; }

/* File offset frame k sits at, per the clock. */
static uint64_t nk_mp3s_frame_byte(const nk_mp3s *s, uint64_t k)
{ return s->first_byte + (k * NK_MP3S_SAMPLES * s->bytes_per_sec) / s->rate; }

/* The frame nearest byte b. Exact for CBR: a frame is hundreds of bytes wide and the clock is good to a
 * byte, so "nearest" is unambiguous. */
static uint64_t nk_mp3s_byte_frame(const nk_mp3s *s, uint64_t b)
{
    if (!s->bytes_per_sec || b < s->first_byte) return 0ull;
    return ((b - s->first_byte) * s->rate + (s->bytes_per_sec * NK_MP3S_SAMPLES) / 2ull) /
           (s->bytes_per_sec * NK_MP3S_SAMPLES);
}

/* ---- the FIFO: samples decoded but not yet asked for ------------------------------------------------ */

static uint32_t fifo_room(const nk_mp3s *s) { return s->fifo_cap - s->fifo_n; }

static void fifo_write(nk_mp3s *s, const int16_t *src, uint32_t n)
{
    uint32_t room = fifo_room(s);
    if (n > room) { s->fifo_dropped += n - room; n = room; }   /* never overwrite unread samples */
    /* fifo_head is the READ cursor; a write lands at the far end of what is in the ring, which is
     * head + fifo_n, not head. The first version of this loop wrote at fifo_head and then advanced it --
     * so the next read started after the samples that had just been written and handed back whatever the
     * ring still held from three frames earlier. Every track came out as scrambled as a result, while
     * every counter in this file stayed perfectly happy: the frame count, the sample count, the byte
     * clock and the seek tests all agreed with each other, because they all agreed on a stream of the
     * right SHAPE. Only comparing the samples against the sectors the MP3 was encoded from says
     * otherwise, which is what analysis/probes/nk_mp3_probe.c does. */
    uint32_t tail = (s->fifo_head + s->fifo_n) & s->fifo_mask;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t p = (tail + i) & s->fifo_mask;
        s->fifo[p * 2] = src[i * 2];            /* L/R interleaved on both sides */
        s->fifo[p * 2 + 1] = src[i * 2 + 1];
    }
    s->fifo_n += n;
}

/* Frames copied to dst from the front of the FIFO. */
static uint32_t fifo_read(nk_mp3s *s, int16_t *dst, uint32_t n)
{
    uint32_t head = s->fifo_head;
    if (n > s->fifo_n) n = s->fifo_n;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t p = (head + i) & s->fifo_mask;
        dst[i * 2] = s->fifo[p * 2];
        dst[i * 2 + 1] = s->fifo[p * 2 + 1];
    }
    s->fifo_head = (head + n) & s->fifo_mask;
    s->fifo_n -= n;
    return n;
}

/* Throw the front of the FIFO away -- a seek landing inside what is already decoded. */
static void fifo_drop(nk_mp3s *s, uint32_t n)
{
    if (n > s->fifo_n) n = s->fifo_n;
    s->fifo_head = (s->fifo_head + n) & s->fifo_mask;
    s->fifo_n -= n;
}

/* ---- the bytes ------------------------------------------------------------------------------------ */

static uint32_t nk_mp3s_avail(const nk_mp3s *s) { return s->chunk_len - s->chunk_pos; }

/* Bring the byte chunk in so that `at` is inside it with room for a frame past it.
 * 1 = bytes in hand, 0 = the read answered nothing (counters say how often). */
static int nk_mp3s_ensure(nk_mp3s *s, uint64_t at)
{
    uint32_t want = s->chunk_ask;
    if (s->chunk_len && at >= s->chunk_off && at + NK_MP3S_REFILL <= s->chunk_off + s->chunk_len)
        return 1;                                       /* already in hand */
    if (s->read(s->user, at, s->chunk, want) != 0) { s->eof_bytes += want; return 0; }
    s->chunk_off = at; s->chunk_len = want; s->chunk_pos = 0;
    s->bytes_read += want;
    return 1;
}

static void nk_mp3s_seek_bytes(nk_mp3s *s, uint64_t at)
{
    s->chunk_len = 0;                                   /* force a read at the new place */
    s->byte_pos = at;
}

/* ---- one frame ------------------------------------------------------------------------------------ */

static int16_t g_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

/* Decode the frame at the byte cursor into the FIFO. Returns the samples produced (0 = the cursor is not
 * on a usable frame; the cursor has still been moved, so the caller always makes progress). */
static uint32_t nk_mp3s_decode_one(nk_mp3s *s)
{
    mp3dec_frame_info_t info;
    memset(&info, 0, sizeof info);
    /* Clear the scratch: minimp3 returns the sample count of a frame it decided not to write (a reservoir
     * it could not restore), and stale bytes from the previous frame would then be played as audio.
     * Silence is the honest version of the same failure. */
    memset(g_pcm, 0, sizeof g_pcm);

    int r = mp3dec_decode_frame(&s->dec->d, s->chunk + s->chunk_pos, nk_mp3s_avail(s), g_pcm, &info);
    if (r <= 0) {
        /* Garbage: frame_bytes is how far it looked for a header. Or a frame that could not be validated,
         * usually one running off the end of the chunk -- which a refill fixes. Step past what it found
         * and let the next attempt see more. After a step that produced no samples the count is no longer
         * trustworthy (those bytes may have held a frame that would not decode), so for a clock we can
         * trust, ask the clock. */
        uint32_t adv = (info.frame_bytes > 0) ? (uint32_t)info.frame_bytes : 1u;
        if (adv > nk_mp3s_avail(s)) adv = nk_mp3s_avail(s);
        s->chunk_pos += adv; s->byte_pos += adv;
        if (s->ready && s->bytes_per_sec && !s->vbr)
            s->frame_sample = nk_mp3s_byte_frame(s, s->byte_pos) * NK_MP3S_SAMPLES;
        s->resyncs++;
        return 0;
    }

    uint32_t fsz = (info.frame_bytes > 0) ? (uint32_t)info.frame_bytes : 1u;
    uint32_t got = (uint32_t)r;
    /* The decoder may have stepped over bytes to find this frame; the header it decoded is not necessarily
     * at the cursor, and an offset recorded against the cursor would be wrong by exactly that step. */
    uint64_t frame_at = s->byte_pos + (uint64_t)info.frame_offset;

    /* While anchoring, a frame that carries a tag is not music -- but it IS a frame, and it has to be
     * decoded. LAME writes its header into an MPEG frame slot at the front of the file, and the frame
     * that comes after it leans on that slot's bit reservoir: step over the slot without decoding it and
     * the decoder starts with an empty reservoir, which minimp3 answers by returning the sample count of
     * a frame it did NOT write (upstream minimp3_ex has the same problem and papers over it with "try to
     * decode up to 255 first frames till samples starts to decode"). Measured here: the first five frames
     * of every track came back as zeros, so every song started with 130 ms of silence. Decode the slot,
     * throw its samples away, and the music that follows is the music. */
    if (!s->ready) {
        uint32_t n = fsz < 96u ? fsz : 96u;
        int tag = 0;
        for (unsigned t = 0; t < sizeof g_tags / sizeof *g_tags; t++)
            if (has4(s->chunk + s->chunk_pos + info.frame_offset, n, g_tags[t])) tag = 1;
        if (tag) {
            s->frames++;
            s->chunk_pos += fsz;
            s->byte_pos += fsz;
            return 0;              /* the decoder has been fed; no samples are counted */
        }
    }

    if (!s->ready) {
        s->rate = (uint32_t)info.hz;
        s->channels = (uint32_t)info.channels;
        s->bitrate_kbps = (uint32_t)info.bitrate_kbps;
        s->bytes_per_sec = s->bitrate_kbps * 125u;               /* kbps * 1000 / 8 */
        s->first_byte = frame_at;
        s->frame_sample = 0ull;
        s->frame_index = 0ull;
        s->ready = 1;
        s->lost = 0;                                /* anchored from the first byte: this position is real */
    } else if ((uint32_t)info.bitrate_kbps != s->bitrate_kbps || !s->bytes_per_sec) {
        s->vbr = 1;                                       /* the clock is not a clock any more */
    }

    /* How good the clock is, measured on every frame against something real: the byte this frame actually
     * sits at, against where the clock says the k-th frame sits. A round trip through the clock's own
     * inverse would be small by construction and measure nothing, which is what the first version of this
     * line did. analysis/probes/nk_mp3_probe.c prints the worst case over a whole track. */
    if (s->bytes_per_sec && !s->vbr) {
        uint64_t est = nk_mp3s_frame_byte(s, s->frame_index);
        uint64_t err = (frame_at > est) ? frame_at - est : est - frame_at;
        if (err > s->map_err_max) s->map_err_max = err;
        s->map_err_sum += err;
        s->map_err_n++;
    }
    s->frame_index++;

    /* The FIFO is stereo; a mono stream is widened here so everything above sees one geometry, which is
     * what a Red Book sector is. */
    if (info.channels > 1) {
        fifo_write(s, g_pcm, got);
    } else {
        uint32_t room = fifo_room(s);
        if (got > room) got = room;
        for (uint32_t i = got; i-- > 0;) { g_pcm[i * 2] = g_pcm[i]; g_pcm[i * 2 + 1] = g_pcm[i]; }
        fifo_write(s, g_pcm, got);
    }
#ifdef NK_MP3_TRACE
    extern int nk_mp3_trace;
    if (nk_mp3_trace < 200)
        fprintf(stderr, "T f=%3d byte=%7llu at=%7llu skip=%2d fsz=%4d got=%4d ready=%d ch=%d hz=%d pcm=%d,%d\n",
                nk_mp3_trace, (unsigned long long)s->byte_pos, (unsigned long long)frame_at,
                info.frame_offset, fsz, got, s->ready, info.channels, info.hz, g_pcm[0], g_pcm[1]);
    nk_mp3_trace++;
#endif
    s->frames++;
    s->frame_sample += got;
    s->samples += got;
    s->chunk_pos += fsz;
    s->byte_pos += fsz;
    return got;
}

/* A hole where a frame should be: silence for a few samples, and the byte cursor moved the matching
 * distance, so the clock does not drift over the hole and the samples that do arrive land where they
 * belong. */
static void nk_mp3s_starve(nk_mp3s *s)
{
    uint32_t n = NK_MP3S_SILENCE;
    uint32_t room = fifo_room(s);
    if (n > room) n = room;
    memset(g_pcm, 0, (size_t)n * 2u * sizeof(int16_t));
    fifo_write(s, g_pcm, n);
    s->silence_frames += n;
    s->frame_sample += n;
    s->lost = 1;            /* the clock moved over bytes that never arrived: what sits at byte_pos now is
                             * arithmetic, not a position in the music. Trusting it is what turned a phone's
                             * stutter into silence for the rest of the track -- nk_mp3s_trusted. */
    uint64_t adv = s->bytes_per_sec ? (NK_MP3S_SILENCE * (uint64_t)s->bytes_per_sec) / s->rate : 512ull;
    s->byte_pos += adv;
    nk_mp3s_seek_bytes(s, s->byte_pos);
}

/* Decode and throw away until sample `want` is reached -- what a seek does. The condition carries
 * `|| !s->ready` because a stream that has not seen its first frame knows nothing yet: not its sample
 * rate, not where the audio starts, not whether the index's clock is the stream's clock. What it drops is
 * only the samples below `want`, so the position after a seek is `want` and not the frame boundary above
 * it. */
static int nk_mp3s_skip_to(nk_mp3s *s, uint64_t want)
{
    uint32_t scanned = 0;
    while (s->frame_sample < want || !s->ready) {
        if (!nk_mp3s_ensure(s, s->byte_pos)) { nk_mp3s_starve(s); scanned = 0; }
        else if (nk_mp3s_decode_one(s) == 0) {
            if (++scanned > NK_MP3S_SCAN_MAX) return -1;   /* that much bitstream with no frame in it */
        } else {
            scanned = 0;
        }
        uint64_t first = s->frame_sample - s->fifo_n;
        if (want > first) fifo_drop(s, (uint32_t)(want - first));
    }
    return 0;
}

/* ---- the entry points ----------------------------------------------------------------------------- */

int nk_mp3s_init(nk_mp3s *s, nk_mp3s_read read, void *user, int16_t *fifo, uint32_t fifo_frames,
                 uint8_t *chunk, uint32_t chunk_bytes)
{
    memset(s, 0, sizeof *s);
    s->dec = calloc(1, sizeof *s->dec);
    if (!s->dec) return -1;
    mp3dec_init(&s->dec->d);
    s->read = read;
    s->user = user;
    s->fifo = fifo; s->fifo_cap = fifo_frames; s->fifo_mask = fifo_frames - 1u;
    s->chunk = chunk; s->chunk_ask = chunk_bytes > NK_MP3S_CHUNK_MIN ? chunk_bytes : NK_MP3S_CHUNK_MIN;
    return 0;
}

void nk_mp3s_free(nk_mp3s *s) { free(s->dec); s->dec = 0; }

/* Start at byte 0 and let the first frame anchor everything: an ID3v2 in front is simply bytes that are
 * not a header, and a LAME/Xing frame slot is skipped by name. */
static int nk_mp3s_anchor(nk_mp3s *s, uint64_t want)
{
    s->chunk_len = 0;
    s->byte_pos = 0ull;
    s->frame_sample = 0ull;
    s->fifo_head = s->fifo_n = 0;
    mp3dec_init(&s->dec->d);
    if (nk_mp3s_skip_to(s, want) != 0) return -1;
    return s->ready ? 0 : -1;
}

int nk_mp3s_seek(nk_mp3s *s, uint64_t want)
{
    /* Everything below counts decoded samples: sample 0 of the file, which is `delay` samples before
     * sample 0 of the material the file was encoded from. The caller speaks the source's timeline, so the
     * padding is added here and taken back in nk_mp3s_pos. */
    s->seeks++;
    want += s->delay;
    uint64_t fifo_first = s->frame_sample - s->fifo_n;

    /* Already in hand: the cheap answer, and the common one when the game re-plays a track it played
     * once in this room. Not when the stream is `lost`: those frames were decoded from a byte position that
     * a hole made an estimate, which is the exact thing `lost` means cannot be trusted -- and serving them
     * is why a dead link took four sectors (~60 ms) of wrong audio to settle instead of coming back on the
     * first one. */
    if (s->ready && want >= fifo_first && want < s->frame_sample) {
        fifo_drop(s, (uint32_t)(want - fifo_first));
        return 0;
    }

    if (!s->ready) return nk_mp3s_anchor(s, want);

    s->fifo_head = s->fifo_n = 0;

    /* Forward: the only thing the game actually does. Bounded, because decoding through a big skip costs
     * the game the time it skipped.
     *
     * Not when the stream is `lost`, and this is the whole reason the flag exists. Decoding forward starts
     * from wherever `byte_pos` says, and across a hole byte_pos was advanced by an estimate -- silence is
     * handed over with a clock that moves even though no bytes were read. Walk forward from an estimate and
     * you land on the right *sample number* from the wrong *bytes*: the scan finds the next header a frame or
     * two on, and from then on every sector is off by that much. Measured on a dead link: 0.06 full scale,
     * which is a song playing slightly out of its own place, forever. The byte clock is the one thing a hole
     * cannot corrupt, so when the cursor is not believed, the clock is used. */
    if (want >= s->frame_sample && want - s->frame_sample < (uint64_t)s->rate * 20ull) {
        mp3dec_init(&s->dec->d);                          /* any jump loses the bit reservoir */
        int r = nk_mp3s_skip_to(s, want);
        s->lost = (r != 0);            /* a skip that ran out of bytes leaves the cursor where it stopped */
        return r;
    }

    /* Backward, or too far forward to decode through: ask the clock where the frame is, land a few frames
     * before that, and step forward to a real header. Exact for CBR. */
    if (s->vbr) s->resyncs++;                             /* reported, and slow: no clock to seek on */
    uint64_t bpf = nk_mp3s_bpf(s);
    uint64_t est = s->vbr ? s->first_byte
                          : nk_mp3s_frame_byte(s, want / NK_MP3S_SAMPLES);
    uint64_t at = (est > s->first_byte + NK_MP3S_BACKOFF * bpf) ? est - NK_MP3S_BACKOFF * bpf
                                                                : s->first_byte;
    nk_mp3s_seek_bytes(s, at);
    s->frame_sample = nk_mp3s_byte_frame(s, at) * NK_MP3S_SAMPLES;
    mp3dec_init(&s->dec->d);
    if (nk_mp3s_skip_to(s, want) != 0) { s->lost = 1; return -1; }
    s->lost = 0;                                            /* landed on the byte clock: believed */
    if (s->frame_sample > want) s->resyncs++;             /* the scan walked past it: say so, play on */
    return 0;
}

uint64_t nk_mp3s_pos(const nk_mp3s *s)
{
    uint64_t p = s->frame_sample - s->fifo_n;
    return (p > s->delay) ? p - s->delay : 0ull;
}

/* 0 = this stream's position is an estimate: it was carried forward over a hole. Asking for a seek when
 * pos != want is not enough on its own, because a hole moves pos by exactly the silence handed over, so a
 * sequential reader sees a perfect match and never asks -- and gets the music from wherever the estimate
 * landed. */
int nk_mp3s_trusted(const nk_mp3s *s) { return !s->lost; }

void nk_mp3s_set_delay(nk_mp3s *s, uint64_t samples) { s->delay = samples; }

uint32_t nk_mp3s_fill(nk_mp3s *s, int16_t *dst, uint32_t nframes)
{
    if (!nframes) return 0;
    /* Anchoring is where the head padding gets thrown away: a fresh stream is asked for public sample 0,
     * which is decoded sample `delay`. */
    if (!s->ready && nk_mp3s_anchor(s, s->frame_sample + s->delay) != 0) { s->lost = 1; return 0; }

    /* Drain what is already decoded, decode a frame when the FIFO is dry, and never both in one pass:
     * that is what keeps the FIFO from growing while a request is being served. */
    uint32_t out = 0, scanned = 0;
    while (out < nframes) {
        uint32_t n = fifo_read(s, dst + (size_t)out * 2u, nframes - out);
        if (n) { out += n; continue; }
        if (!nk_mp3s_ensure(s, s->byte_pos)) { nk_mp3s_starve(s); scanned = 0; continue; }
        if (nk_mp3s_decode_one(s) == 0) {
            if (++scanned > NK_MP3S_SCAN_MAX) break;      /* nothing decodable in 32 KB: give up, honestly */
        } else {
            scanned = 0;
        }
    }
    if (out < nframes) {                                   /* nothing left to take: silence, counted */
        memset(dst + (size_t)out * 2u, 0, (size_t)(nframes - out) * 2u * sizeof(int16_t));
        s->silence_frames += nframes - out;
        s->frame_sample += nframes - out;                  /* the clock moves anyway: the caller did */
        s->lost = 1;                                       /* ... over bytes that were never read */
        out = nframes;
    }
    return out;
}

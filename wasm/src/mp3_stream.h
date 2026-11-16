/* mp3_stream.h -- an MP3 bitstream over a byte-read callback: sequential decode, analytic CBR seek,
 * stereo int16 out.  docs/463.  Pure bitstream code: no network, no filesystem, no emscripten, so the
 * same file compiles into the wasm module and into analysis/probes/nk_mp3_probe.c.
 *
 * WHY A CALLBACK AND NOT A FILE
 * -----------------------------
 * The browser port feeds this from HTTP Range windows (wasm/src/wasm_disc.c): it asks for the bytes under
 * the decode cursor and gets a memcpy out of a window that is already on its way. So the stream is given
 * "give me n bytes at an offset" and never assumes it can seek, never assumes the bytes are there, and
 * never assumes a frame it wants is whole.
 *
 * WHY ANALYTIC SEEK AND NOT A SEEK TABLE
 * --------------------------------------
 * A CBR MP3 is a clock: every frame is bitrate/8 bytes and 1152 samples, so sample S lives at byte
 * first_byte + S * bytes_per_sec / rate -- an estimate within one byte. Landing on that byte and scanning
 * forward to a frame header is exact, because from that header on every sample is counted. A VBR file has
 * no such map (upstream's minimp3_ex builds one by scanning the whole stream, which here would mean
 * downloading a track to hear one scene), so a stream whose frames are not all one bitrate is reported
 * and refuses a backward seek; it keeps playing forward, which is all the game ever asks of it.
 *
 * WHAT COMES OUT
 * --------------
 * int16 stereo at the stream's own rate, in absolute sample numbers: seek(S) then fill(n) yields exactly
 * the samples S..S+n-1, where sample 0 is the first AUDIO frame -- ID3v2 tags and Xing/Info/LAME header
 * frames are skipped on the way in, the way a gapless decoder skips them.
 */
#ifndef NK_MP3_STREAM_H
#define NK_MP3_STREAM_H

#include <stdint.h>

/* "Give me n bytes of the stream at byte offset off into dst." 0 = they are here, -1 = they are not.
 * A -1 is not an error and does not end anything: the stream emits silence for the frames those bytes
 * would have carried and moves on, with its byte-to-sample map intact. */
typedef int (*nk_mp3s_read)(void *user, uint64_t off, void *dst, uint32_t n);

typedef struct nk_mp3s_dec nk_mp3s_dec;      /* the vendored decoder's state, private to mp3_stream.c */

typedef struct {
    nk_mp3s_read read;                       /* who owns the bytes */
    void        *user;
    nk_mp3s_dec  *dec;

    /* what the first audio frame said; 0 until a frame has been seen */
    uint32_t rate, channels, bitrate_kbps, bytes_per_sec;
    uint64_t first_byte;                     /* byte offset of the first audio frame (past tags and Xing) */
    int      vbr;                            /* 1 = frames of more than one bitrate have been seen */

    /* the byte chunk currently in hand */
    uint64_t byte_pos;                       /* file offset of the MP3 frame about to be decoded */
    uint64_t chunk_off;                      /* file offset of chunk[0] */
    uint32_t chunk_len, chunk_pos, chunk_ask;
    uint8_t *chunk;

    /* the decoded side: a stereo FIFO indexed by absolute sample number.
     * `frame_sample` is the sample number of the frame at byte_pos; the FIFO holds the frames decoded
     * before it and not yet handed over, so the next sample this stream produces is
     * frame_sample - fifo_n. */
    int16_t *fifo;                           /* fifo_cap frames, L/R interleaved, a ring */
    uint32_t fifo_cap, fifo_mask, fifo_head, fifo_n;
    uint64_t frame_sample;                   /* decoded samples produced before the byte cursor */
    int      lost;                           /* 1 when `pos` can no longer be believed. Silence is fed to
                                              * the caller through a hole, and the clock moves even though
                                              * nothing was decoded -- so the byte cursor behind it is now
                                              * an estimate, and continuing to decode from it plays the
                                              * music from the wrong place. A caller that asks "am I where
                                              * I said I was?" gets no for an answer and re-anchors off the
                                              * byte clock, which is the one thing a hole cannot corrupt. */
    uint64_t frame_index;                    /* which audio frame the cursor is on, counted from the first
                                              * one the stream counted: the clock's own index, and what
                                              * map_err below is measured against */

    /* counters, for a log line and for the probe */
    uint64_t bytes_read, frames, samples, silence_frames, seeks, resyncs, eof_bytes;
    uint64_t fifo_dropped;                    /* samples the FIFO could not hold -- should stay 0 */
    uint64_t map_err_max;                    /* worst |actual frame byte - analytic estimate|, in bytes */
    uint64_t map_err_sum, map_err_n;
    uint64_t delay;                          /* samples the encoder put in front of the music. An encoder
                                              * pads the head -- it has to, its filterbank has a state --
                                              * and only a decoder that reads its tag knows to throw the
                                              * padding away. Sample numbers in and out of this module are
                                              * on the timeline the *source* material is on, so a seek to
                                              * sample S returns the samples that were in front of the
                                              * encoder, not the ones after its padding: the padding is
                                              * dropped on the way in. 0 = nothing dropped, and that is
                                              * what an unmeasured stream gets. Set it before the first
                                              * seek or fill. */
    int      ready;                          /* 1 = anchored: rate/first_byte are real */
    int      no_bytes;                       /* the first audio frame's bytes never came */
} nk_mp3s;

/* fifo_frames stereo frames of FIFO, and chunk_bytes of bitstream buffer, are both the caller's.
 * Returns 0 on success; -1 if the decoder could not be allocated. */
int  nk_mp3s_init(nk_mp3s *s, nk_mp3s_read read, void *user, int16_t *fifo, uint32_t fifo_frames,
                  uint8_t *chunk, uint32_t chunk_bytes);
void nk_mp3s_free(nk_mp3s *s);

/* Anchor the stream so the next sample produced is `want` (0 = the first audio frame).
 * 0 = ok, -1 = it cannot be anchored at all (no bytes, or not an MP3). */
int  nk_mp3s_seek(nk_mp3s *s, uint64_t want);

/* Write n stereo frames to dst. Always returns n: what could not be decoded is zeros, counted in
 * s->silence_frames. Returns 0 only if the stream cannot be anchored at all. */
uint32_t nk_mp3s_fill(nk_mp3s *s, int16_t *dst, uint32_t nframes);

/* The head padding, in samples. See the `delay` field: it is measured offline against the material the
 * file was encoded from (analysis/probes/nk_mp3_probe.c finds it by sliding the decoded head across the
 * source until the two line up), because nothing in an MP3 states it in a form a decoder that does not
 * read tags can use. Set before the first seek or fill; changing it later only moves the timeline. */
void nk_mp3s_set_delay(nk_mp3s *s, uint64_t samples);

/* The sample number the next nk_mp3s_fill starts at. A caller that plays a track forward uses it to tell
 * "keep going" from "seek": the second one resets the decoder, and a decoder reset every sector is heard
 * as the bit reservoir throwing away the head of each frame. */
uint64_t nk_mp3s_pos(const nk_mp3s *s);

/* 0 = `pos` is an estimate carried across a hole, so a caller that only seeks when pos != want will never
 * seek and will keep playing from wherever the estimate landed. 1 = the position came from a real anchor
 * or a real seek. */
int nk_mp3s_trusted(const nk_mp3s *s);

#endif /* NK_MP3_STREAM_H */

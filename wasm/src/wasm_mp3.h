/* wasm_mp3.h -- CD-DA out of compressed bytes instead of Red Book sectors, for the browser port.
 * docs/463.  See wasm_mp3.c. */
#ifndef NK_WASM_MP3_H
#define NK_WASM_MP3_H

#include <stdint.h>

/* The compressed alternative to <cd base>/trackNN.bin lives in `base` (e.g. "data/audio"), and the one
 * file there, tracks.tsv, is what makes it usable: it names, per track, the encoded byte length, the
 * Red Book length, the bitrate, the sample rate and the decoded sample count. Without it the port does
 * not know how long a lossy track is (the encoded length says nothing about the music in it) and falls
 * back to raw sectors for every track. Call before cd_iso_open. `on` = 0 turns the probe off entirely. */
void nk_mp3_install(const char *base, int on);

/* 1 = this track has lossy bytes the index says are playable. */
int nk_mp3_has(uint32_t track);

/* What cd_iso should be told this track is, in Red Book bytes -- from the index, so the geometry the
 * game sees does not change when the bytes underneath it do. 0 = unknown. */
uint64_t nk_mp3_redbook_bytes(uint32_t track);

/* Answer `n` bytes of a Red Book read (byte offset `off` inside the track, n a multiple of 2352) by
 * decoding compressed bytes. 0 = the sector is in dst, -1 = this track has no more bytes. Bytes the
 * network would not deliver come back as silence, counted, exactly as the raw path answers them. */
int nk_mp3_pread(uint32_t track, uint64_t off, void *dst, uint32_t n);

/* The sample rate of what is being decoded now (0 until a frame has been seen), so the host can mark its
 * ring: a 48 kHz encode would otherwise play at 44.1 and stretch. */
uint32_t nk_mp3_rate(void);

/* 1 = an index was read and holds at least one track. */
int nk_mp3_ready(void);
const char *nk_mp3_source(void);           /* "" until installed, else the base it was given */

/* decoded frames, bytes of mp3 fetched, seeks, holes, worst clock error in bytes, decode time in us. */
void nk_mp3_stats(uint64_t *frames, uint64_t *bytes, uint64_t *seeks, uint64_t *silence,
                  uint64_t *map_err_max, uint64_t *decode_us);

/* What one Red Book read cost the game from the inside -- the decode AND the seek that may precede it, both
 * of which sit on MSCDEX's path. Calls/total answer "does this cost the game much CPU", and max answers
 * "is this what a stutter is", which the total never will. */
void nk_mp3_slow_split(uint32_t *track_start, uint32_t *steady);

void nk_mp3_pread_stats(uint32_t *calls, uint32_t *us_total, uint32_t *us_max,
                        uint32_t *seek_calls, uint32_t *seek_us_total, uint32_t *seek_us_max,
                        uint32_t *over_2ms, uint32_t *over_8ms);

#endif /* NK_WASM_MP3_H */

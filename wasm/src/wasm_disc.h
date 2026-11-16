/* wasm_disc.h -- the browser host's CD-DA byte source.  See wasm_disc.c. */
#ifndef NK_WASM_DISC_H
#define NK_WASM_DISC_H
#include <stdint.h>

/* Install the Range-read byte source into cd_iso (call before cd_iso_open). `base` is the directory
 * holding trackNN.bin, e.g. "data/cd" relative to the page. */
void nk_disc_install(const char *base);

/* The same sliding-window Range service, addressed by URL instead of by track number, for a source that
 * is not Red Book sectors -- the lossy CD-DA of wasm/src/wasm_mp3.c, whose bytes are an MP3 and whose
 * read pointer is a decode cursor. One implementation on purpose: the read-ahead, the backoff, the
 * "a network that blinks is a hole in the music and nothing else" rule and the never-stop-the-game rule
 * were the expensive parts, and a second source gets them for free.
 *
 * `lead` is how many bytes of the file to keep buffered ahead of the read pointer before asking for
 * another window, and the window itself is derived from it (see disc_pread_url). With `report_fail`, a
 * read the network did not answer comes back -1 instead of silence; without it, silence -- which is the
 * rule a Red Book read follows, because the game's timeline cannot stop for one. */
int  nk_disc_read(const char *url, uint64_t off, void *dst, uint32_t n, uint32_t lead, int report_fail);

/* HEAD a URL and cache the answer. 0 = answered (*out == 0 means present, length unknown), -1 = not
 * there. Retried, because "this track is not there" is the one answer the port cannot take back. */
int  nk_disc_size_url(const char *url, uint64_t *out_bytes);

/* Seed the same cache from a fact that is already known -- the byte length an index states -- so a source
 * built from an index never has to HEAD anything, and never reads past the end of a file it is only
 * reading a window of. (Reading past the end is not harmless here: this repo's own server answers it by
 * closing the connection, and every long run used to die at the last sector of the show's track.) */
void nk_disc_size_set(const char *url, uint64_t bytes);

/* GET a small file in one piece and NUL-terminate it in dst (an index, a manifest: nothing the sliding
 * window exists for). Byte count, or -1. */
int  nk_disc_get_text(const char *url, char *dst, uint32_t cap);

/* Counters for the boot log and the status block. Any may be NULL.
 * `waits` counts the reads the game had to stop for (an awaited Range read), `wait_max_us` the worst
 * such stop, and `silence` the bytes of silence substituted for bytes the network would not deliver. */
void nk_disc_stats(uint32_t *reads, uint32_t *read_bytes, uint32_t *fetches, uint32_t *fetch_bytes,
                   uint32_t *fails, uint32_t *waits, uint32_t *wait_max_us, uint32_t *silence);

/* Reads answered out of bytes the page downloaded before the game started (preload mode). A run with
 * everything preloaded reads pre=everything, fetched=0, fails=0 and holes=0. */
void nk_disc_pre_stats(uint32_t *reads, uint32_t *bytes);

/* Answer every read of an audio track with silence and fetch nothing (the page's music box, unchecked).
 * Distinct from refusing the MP3 path, which streams raw sectors instead -- more bytes, not fewer. */
void nk_disc_silent_audio(int on);

/* Count silence the disc layer did not make itself (see nk_disc_silence_add in wasm_disc.c). */
void nk_disc_silence_add(uint32_t bytes);

#endif

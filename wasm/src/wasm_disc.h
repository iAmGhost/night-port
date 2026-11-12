/* wasm_disc.h -- the browser host's CD-DA byte source.  See wasm_disc.c. */
#ifndef NK_WASM_DISC_H
#define NK_WASM_DISC_H
#include <stdint.h>

/* Install the Range-read byte source into cd_iso (call before cd_iso_open). `base` is the directory
 * holding trackNN.bin, e.g. "data/cd" relative to the page. */
void nk_disc_install(const char *base);

/* Counters for the boot log and the status block. Any may be NULL. */
void nk_disc_stats(uint32_t *reads, uint32_t *read_bytes, uint32_t *fetches, uint32_t *fetch_bytes,
                   uint32_t *fails);

#endif

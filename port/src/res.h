#ifndef NIGHT_RES_H
#define NIGHT_RES_H

#include <stdint.h>
#include <stddef.h>

/* ---------- NIGHT.RES container (docs/02-container.md) ----------
 * [u32 count]
 * [count x 0x14 dir bytes, reverse-XOR obfuscated:
 *      for i = len-1 .. 1:  dir[i] ^= dir[i-1]
 *      dir[0] ^= 0x20
 *  entry = name[12] | u32 size | u32 offset(absolute)]
 * [packed blobs]
 */
typedef struct {
    char     name[13];
    uint32_t size;
    uint32_t offset;
} ResEntry;

typedef struct {
    uint8_t  *data;   /* whole file mapped in RAM */
    uint32_t  size;
    uint32_t  count;
    ResEntry *entries;
} Res;

int  res_open(const char *path, Res *r);
void res_close(Res *r);
const ResEntry *res_find(const Res *r, const char *name);
const uint8_t *res_blob(const Res *r, const ResEntry *e);

/* ---------- .pcx (docs/03-formats.md §1) ----------
 * 128B header (xmax@8, ymax@10 -> 639/479), RLE data, 768B BGR tail.
 * RLE: b<0xC0 literal; else run (b&0x3F) copies of next byte.
 * Palette channel = stored_byte << 2.
 */
typedef struct {
    int      w, h;
    uint8_t *idx;          /* w*h palette indices */
    uint8_t  pal[256 * 3]; /* RGB, expanded (<<2) */
} Pcx;

int  pcx_decode(const uint8_t *d, size_t n, Pcx *p);
void pcx_free(Pcx *p);

/* ---------- .spr (docs/03-formats.md §2) ----------
 * [u32 count]; per sprite: 12 u32 hdr + u32 size + size bytes.
 * per row: [u8 nseg][ (u8 skip, u8 len, u8 px[len]) x nseg ]
 * Draw rule (verified vs reference sheets): a row is first filled with the
 * background value (0 = black for the sheets; composite for gameplay), then
 * segments are written over it.
 */
typedef struct {
    uint32_t      w, h;
    uint32_t      hdr[12];
    const uint8_t *raw;  /* pixel-data section (size bytes) */
    uint32_t      size;
} Spr;

int  spr_decode(const uint8_t *d, size_t n, Spr **out, int *count); /* out[count], caller frees */
void spr_free(Spr *s, size_t count);
void spr_draw_row(uint8_t *dst, int dst_w, uint8_t bg, const Spr *s, int y);

/* ---------- .voc (docs/04-sound.md) ----------
 * 26B header ("Creative Voice File" 19B + 7B metadata), then tag stream:
 * [u8 type][u24 LE len][payload]
 * 0x01 voice: [u8 handler][u8 rate_idx][8-bit unsigned PCM x (len-2)]
 * 0x00 stop (end), 0x08 reserved (skipped), others skipped.
 */
typedef struct {
    int       n_samples;
    uint8_t  *pcm;    /* concatenated voice samples */
    int       n_voice_blocks;
} Voc;

int  voc_decode(const uint8_t *d, size_t n, Voc *v);
void voc_free(Voc *v);

/* ---------- .fli (docs/03-formats.md §11) ---------- */
typedef struct {
    const uint8_t *data;
    uint32_t       size;
    uint32_t       frame_count, w, h;
    uint32_t       pos;
    uint8_t       *pal;      /* 256*3 RGB, <<2-expanded */
    int            pal_set;
} Fli;

int  fli_open(const uint8_t *d, uint32_t n, Fli *f);
void fli_free(Fli *f);
int  fli_next(Fli *f, uint8_t *fb);   /* consume next 0xF1FA chunk (setup or frame); 0 = done */

/* ---------- WAV (CD-DA tracks extracted by scripts/cdda2wav.py) ---------- */
typedef struct {
    int     rate, channels, bits;
    int16_t *samples;   /* interleaved */
    size_t  nframes;
} Wav;

int  wav_open(const uint8_t *d, size_t n, Wav *w);
void wav_free(Wav *w);

/* ---------- minimal dependency-free PNG writer (8-bit gray / RGB) ---------- */
int png_write_gray(const char *path, int w, int h, const uint8_t *px);
int png_write_rgb (const char *path, int w, int h, const uint8_t *px);

#endif

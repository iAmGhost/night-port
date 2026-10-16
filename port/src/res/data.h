#ifndef NIGHT_DATA_H
#define NIGHT_DATA_H

#include "res.h"

/* .act (docs/03-formats.md §3): [u32 count] + records of 109 + 10n bytes */
typedef struct {
    int       n;            /* beat count */
    uint16_t *frames;       /* sprite index per beat */
    uint32_t *param_a;      /* per-beat params A (mostly zero) */
    uint32_t *param_b;      /* per-beat params B (mostly zero) */
    uint8_t   hdr[109];     /* raw record incl. leading u32 n */
} ActRec;

typedef struct {
    int      count;
    ActRec  *recs;
    int      total_beats;
} Act;

int  act_decode(const uint8_t *d, size_t n, Act *a);
void act_free(Act *a);

/* .stg (docs/03-formats.md §5): [u32 count] + count x 81-byte records */
typedef struct {
    uint8_t  tag;
    uint32_t p[20];
} StgRec;

typedef struct {
    int     count;
    StgRec *recs;
} Stg;

int  stg_decode(const uint8_t *d, size_t n, Stg *s);
void stg_free(Stg *s);

/* .wal (docs/03-formats.md §4): recursive node chain */
typedef struct {
    uint8_t       a, b;
    uint32_t      p0, p1, p2, p3, p4, p5;
    size_t        grid_n;
    uint8_t       *grid;    /* u8 tile indices (owned copy) */
    uint8_t       pal[768];
    uint32_t      nmark;
    uint16_t      *marks;   /* owned copy */
} WalNode;

typedef struct {
    int       count;
    WalNode  *nodes;
} Wal;

int  wal_decode(const uint8_t *d, size_t n, Wal *w);
void wal_free(Wal *w);

/* .key (docs/03-formats.md §7): ASCII CRLF text */
typedef struct {
    int  frame_count;
    int  n_tokens;   /* key-token count on the combo line */
    char *raw;       /* whole file, NUL-terminated */
} Key;

int  key_decode(const uint8_t *d, size_t n, Key *k);
void key_free(Key *k);

#endif

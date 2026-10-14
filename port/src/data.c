#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

int act_decode(const uint8_t *d, size_t n, Act *a) {
    memset(a, 0, sizeof *a);
    if (n < 4) return -1;
    uint32_t count = rd32(d);
    a->count = (int)count;
    a->recs = calloc(count ? count : 1, sizeof *a->recs);
    size_t off = 4;
    for (uint32_t i = 0; i < count; i++) {
        if (off + 109 > n) return -1;
        ActRec *r = &a->recs[i];
        r->n = (int)rd32(d + off);
        memcpy(r->hdr, d + off, 109);
        if (off + 109 + (size_t)r->n * 10 > n) return -1;
        const uint8_t *fp = d + off + 109;
        r->frames = malloc((size_t)r->n * sizeof(uint16_t));
        r->param_a = malloc((size_t)r->n * sizeof(uint32_t));
        r->param_b = malloc((size_t)r->n * sizeof(uint32_t));
        for (int k = 0; k < r->n; k++) {
            r->frames[k] = rd16(fp + k * 2);
            r->param_a[k] = rd32(fp + (size_t)r->n * 2 + k * 4);
            r->param_b[k] = rd32(fp + (size_t)r->n * 6 + k * 4);
        }
        a->total_beats += r->n;
        off += 109 + (size_t)r->n * 10;
    }
    if (off != n) return -1;
    return 0;
}

void act_free(Act *a) {
    for (int i = 0; i < a->count; i++) {
        free(a->recs[i].frames);
        free(a->recs[i].param_a);
        free(a->recs[i].param_b);
    }
    free(a->recs);
    memset(a, 0, sizeof *a);
}

/* ---------- .stg (docs/03-formats.md §5): [u32 count] + count x 81B ---------- */
int stg_decode(const uint8_t *d, size_t n, Stg *s) {
    memset(s, 0, sizeof *s);
    if (n < 4) return -1;
    uint32_t count = rd32(d);
    if (4 + (size_t)count * 81 != n) return -1; /* exact size invariant */
    s->count = (int)count;
    s->recs = malloc((size_t)count * sizeof(StgRec));
    const uint8_t *p = d + 4;
    for (uint32_t i = 0; i < count; i++) {
        s->recs[i].tag = p[0];
        for (int k = 0; k < 20; k++) s->recs[i].p[k] = rd32(p + 1 + k * 4);
        p += 81;
    }
    return 0;
}

void stg_free(Stg *s) { free(s->recs); s->recs = NULL; }

/* ---------- .wal (docs/03-formats.md §4): node chain ---------- */
int wal_decode(const uint8_t *d, size_t n, Wal *w) {
    memset(w, 0, sizeof *w);
    if (n < 4) return -1;
    uint32_t count = rd32(d);
    w->count = (int)count;
    w->nodes = calloc(count ? count : 1, sizeof *w->nodes);
    size_t off = 4;
    for (uint32_t i = 0; i < count; i++) {
        WalNode *nd = &w->nodes[i];
        if (off + 2 + 24 > n) return -1;
        nd->a = d[off]; nd->b = d[off + 1];
        nd->p0 = rd32(d + off + 2); nd->p1 = rd32(d + off + 6);
        nd->p2 = rd32(d + off + 10); nd->p3 = rd32(d + off + 14);
        nd->p4 = rd32(d + off + 18); nd->p5 = rd32(d + off + 22);
        off += 2 + 24;
        size_t gw = (nd->p1 & 1) ? (nd->p3 - nd->p1) * 320 : (nd->p2 - nd->p0) * 200;
        if (off + gw > n) return -1;
        nd->grid_n = gw;
        nd->grid = d + off;
        off += gw;
        if (off + 768 > n) return -1;
        memcpy(nd->pal, d + off, 768);
        off += 768;
        if (off + 4 > n) return -1;
        nd->nmark = rd32(d + off);
        off += 4;
        if (off + (size_t)nd->nmark * 2 > n) return -1;
        nd->marks = (const uint16_t *)(d + off);
        off += (size_t)nd->nmark * 2;
    }
    if (off != n) return -1;
    return 0;
}

void wal_free(Wal *w) { free(w->nodes); w->nodes = NULL; }

/* ---------- .key (docs/03-formats.md §7): ASCII CRLF text ----------
 * line 1: frame count; blank line(s); key-token line (space separated);
 * then numeric field lines. */
int key_decode(const uint8_t *d, size_t n, Key *k) {
    memset(k, 0, sizeof *k);
    k->raw = malloc(n + 1);
    memcpy(k->raw, d, n);
    k->raw[n] = 0;
    k->frame_count = atoi(k->raw);
    char **lines = malloc((n + 1) * sizeof *lines);
    int nl = 0;
    char *p = k->raw;
    lines[nl++] = p;
    while ((p = memchr(p, '\n', n - (p - k->raw)))) { *p = 0; lines[nl++] = p + 1; p++; }
    for (int i = 0; i < nl; i++) {
        const char *t = lines[i];
        while (*t == ' ' || *t == '\r') t++;
        if (!*t) continue;
        int has_letter = 0;
        for (const char *c = t; *c; c++)
            if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z')) { has_letter = 1; break; }
        if (has_letter) {
            for (const char *c = t; *c; c++)
                if (*c == ' ') k->n_tokens++;
            k->n_tokens++; /* last token */
            break;
        }
    }
    free(lines);
    return 0;
}

void key_free(Key *k) { free(k->raw); memset(k, 0, sizeof *k); }

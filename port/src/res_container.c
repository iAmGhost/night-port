#include "res.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int res_open(const char *path, Res *r) {
    memset(r, 0, sizeof *r);
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "res_open: cannot open %s\n", path); return -1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 4 + 0x14) { fclose(f); return -1; }
    uint8_t *data = malloc((size_t)sz);
    if (fread(data, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); free(data); return -1; }
    fclose(f);

    uint32_t count = rd32(data);
    size_t dlen = (size_t)count * 0x14;
    if (4 + dlen > (size_t)sz) { free(data); return -1; }
    uint8_t *dir = data + 4;
    for (size_t i = dlen - 1; i >= 1; i--) dir[i] ^= dir[i - 1];
    dir[0] ^= 0x20;

    ResEntry *e = malloc(dlen);
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *p = dir + (size_t)i * 0x14;
        memcpy(e[i].name, p, 12);
        e[i].name[12] = 0;
        e[i].size   = rd32(p + 0x0C);
        e[i].offset = rd32(p + 0x10);
    }
    r->data = data; r->size = (uint32_t)sz; r->count = count; r->entries = e;
    return 0;
}

void res_close(Res *r) {
    free(r->entries);
    free(r->data);
    memset(r, 0, sizeof *r);
}

const ResEntry *res_find(const Res *r, const char *name) {
    for (uint32_t i = 0; i < r->count; i++)
        if (strcmp(r->entries[i].name, name) == 0) return &r->entries[i];
    return NULL;
}

const uint8_t *res_blob(const Res *r, const ResEntry *e) {
    if (e->offset + e->size > r->size) return NULL;
    return r->data + e->offset;
}

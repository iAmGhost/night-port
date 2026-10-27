/* rig_sink: the port's half of the differential rig's state oracle.
 *
 * scripts/audit/gen_headers.py turns the shipped Ghidra symbol database into rig_table.h: every port
 * global that claims a DS-group address of NIGHT.EXE, with the size the guest's own accesses imply and
 * the size the port's declaration implies. This file dumps that table on a fixed wall-clock cadence, so
 * the same table can be read from the original's *physical memory* through the emulator API
 * (scripts/rig/dump_original.py) and diffed per variable, per event window.
 *
 * Nothing here interprets the values: interpretation is the diff tool's job, so a wrong port symbol
 * mapping shows up as a bogus diff instead of being hidden by translation.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "f/rig_cells.h"

#define RIG_MAX 512

static FILE *g_bin, *g_meta, *g_marks;
static long g_period_ms = 100;
static long long g_next_ms;
static unsigned g_tick;
static uint32_t g_rec_bytes;
static uint32_t g_off[RIG_MAX];
static int g_n;
#define RIG_REC 64              /* bytes of the object a pointer cell refers to, also dumped */
static uint32_t g_recoff[RIG_MAX];

void rig_init(const char *dir, int hz) {
    char path[1024];
    if (hz > 0) g_period_ms = 1000 / hz;
    mkdir(dir, 0777);   /* EEXIST is fine */
    snprintf(path, sizeof path, "%s/cells.bin", dir);
    g_bin = fopen(path, "wb");
    snprintf(path, sizeof path, "%s/meta.json", dir);
    g_meta = fopen(path, "w");
    snprintf(path, sizeof path, "%s/markers.jsonl", dir);
    g_marks = fopen(path, "w");
    if (!g_bin || !g_meta) {
        fprintf(stderr, "rig_sink: cannot open %s\n", dir);
        exit(1);
    }
    g_n = rig_cell_count();
    if (g_n > RIG_MAX) g_n = RIG_MAX;
    uint32_t off = 0;
    for (int i = 0; i < g_n; i++) {
        g_off[i] = off;
        off += rig_cell_at(i)->host_size;
    }
    g_rec_bytes = off;

    uint32_t hdr[4] = {0x4352474eu /* NGRC */, 1u, g_rec_bytes, (uint32_t)g_n};
    fwrite(hdr, sizeof hdr, 1, g_bin);

    fprintf(g_meta, "{\"record_bytes\":%u,\"period_ms\":%ld,\"cells\":[", g_rec_bytes, g_period_ms);
    for (int i = 0; i < g_n; i++) {
        const struct rig_cell *c = rig_cell_at(i);
        fprintf(g_meta, "%s{\"ds\":%u,\"name\":\"%s\",\"guest_size\":%u,\"host_size\":%u,"
                       "\"ptr\":%u,\"offset\":%u,\"rec_offset\":%u,\"rec_bytes\":%d}",
                i ? "," : "", c->ds, c->name, c->guest_size, c->host_size, c->ptr, g_off[i],
                g_recoff[i], RIG_REC);
    }
    fprintf(g_meta, "]}\n");
    fflush(g_meta);
}

/* One record per cadence tick. `frame` is the port's presented-frame counter, `audio_bytes` the
 * PCM bytes handed to the audio device (the audio oracle's window bound); `screen_hash` stays zero
 * until the media oracle fills it in. */
void rig_poll(long t_ms, unsigned long long frame, unsigned long long audio_bytes) {
    if (!g_bin || t_ms < g_next_ms) return;
    g_next_ms = t_ms + g_period_ms;

    /* field order chosen so the layout has no padding: the reader (scripts/rig/diff_state.py) uses a
     * packed struct and must not have to guess the compiler's alignment decisions. */
    struct { int64_t t_ms; uint64_t frame, audio_bytes, screen_hash; uint32_t tick, n; } h = {
        t_ms, frame, audio_bytes, 0, g_tick, (uint32_t)g_n
    };
    fwrite(&h, sizeof h, 1, g_bin);
    unsigned char *p = malloc(g_rec_bytes);
    if (!p) return;
    memset(p, 0, g_rec_bytes);
    for (int i = 0; i < g_n; i++) {
        const struct rig_cell *c = rig_cell_at(i);
        memcpy(p + g_off[i], c->host, c->host_size);
        if (g_recoff[i] != 0xffffffffu && c->host_size == 8) {
            const void *obj = *(const void *const *)c->host;
            if (obj) memcpy(p + g_recoff[i], obj, RIG_REC);   /* else: stays zero = "null pointer" */
        }
    }
    fwrite(p, g_rec_bytes, 1, g_bin);
    free(p);
    g_tick++;
    if ((g_tick & 15) == 0) fflush(g_bin);
}

/* Windows for the diff tool: named events (scripted keys, scene changes) stamped on the same clock
 * as the records. */
void rig_marker(long t_ms, const char *fmt, ...) {
    if (!g_marks) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fprintf(g_marks, "{\"tick\":%u,\"t_ms\":%ld,\"what\":\"%s\"}\n", g_tick, t_ms, buf);
    fflush(g_marks);
}

void rig_close(void) {
    if (g_bin) fflush(g_bin);
}

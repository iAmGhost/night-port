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
#include "f/f.h"

#define RIG_ENTS 12   /* entity-list slots dumped per sample */                      /* g_heap_base / g_heap_arena_size (docs/107 heap-relative dump) */

#define RIG_MAX 512

static FILE *g_bin, *g_meta, *g_marks;
/* CD-DA witness. The port plays its music straight off the disc through a separate SDL stream, so a
 * witness on the DSP/PCM path alone cannot see the music at all -- and in the shipped game the music is
 * CD-DA as well, which is what the container's mixer capture records. cdda.raw holds the s16le stereo
 * 44100 Hz bytes the port fed to the device, and cdda.jsonl places them on the nominal timeline and
 * marks every play/stop with the track index, LBA and length, so a missing or wrong track is a schedule
 * difference that can be read off directly instead of inferred from loudness. */
static FILE *g_cdda, *g_cdda_idx;
static uint64_t g_cdda_bytes;
static long long g_cdda_next_ms;
static long g_period_ms = 100;
static long long g_next_raw_ms;
/* Script anchors move the timeline origin: the playtest script rebases its keys onto shipped game
 * events, and the shipped side's rig t=0 is its own anchor. Without the same rebase here, the port's
 * first phase is "boot" while the shipped side's first phase is "title card up", which made every DS
 * cell look wrong at P0. */
static long long g_t_shift_ms;

/* The script slides the remaining events by `slip` so a nominal drive time X now fires on the wall clock
 * at X + slip; to stamp the record with X itself the shift must *undo* the slip. Adding it instead
 * double-counts, which pushed the port's stamps 140 s away from the drive's nominal times. */
void rig_rebase(long long slip_ms) { g_t_shift_ms -= slip_ms; }
static unsigned g_tick;
static uint32_t g_rec_bytes;
static uint32_t g_off[RIG_MAX];
static uint32_t g_ent_base;      /* offset of the first entity-list slot in the blob */
static int g_n;
#define RIG_REC 0x160          /* bytes of the object a pointer cell refers to, also dumped: the
                                entity record the state oracle reads (EXP +0x2b, score +0x52,
                                award +0xbb, position +0x56/+0x73), up to the width-adapted
                                link fields at +0x13f/+0x143, which are not comparable */
static uint32_t g_recoff[RIG_MAX];

/* Read the current value of one DS-claiming global by its DS address, little-endian, up to 4 bytes.
 * Used by the playtest script's `wait ds ADDR OP VAL`, so both sides of the rig can anchor on the same
 * shipped DS event (a resource name cannot be used: every resource name is resident in DS all the time,
 * measured -- 435 name-shaped strings present at the title and unchanged after entering the game). */
int rig_ds_read(uint32_t ds, uint32_t *out) {
    for (int i = 0; i < rig_cell_count(); i++) {
        const struct rig_cell *c = rig_cell_at(i);
        if (c->ds != ds) continue;
        unsigned n = c->host_size < 4 ? c->host_size : 4;
        const unsigned char *b = (const unsigned char *)c->host;
        uint32_t v = 0;
        for (unsigned k = 0; k < n; k++) v |= (uint32_t)b[k] << (8 * k);
        *out = v;
        return (int)c->host_size;
    }
    return 0;
}

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
    snprintf(path, sizeof path, "%s/cdda.raw", dir);
    g_cdda = fopen(path, "wb");
    snprintf(path, sizeof path, "%s/cdda.jsonl", dir);
    g_cdda_idx = fopen(path, "w");
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
    /* Each pointer cell also gets a block of the object it points at, laid out AFTER the cell area.
     * Without this every record landed at offset 0 -- on top of the first cell -- and the entity-level
     * oracles compared the wrong bytes (docs/105). */
    uint32_t rec_off = off;
    for (int i = 0; i < g_n; i++) {
        const struct rig_cell *c = rig_cell_at(i);
        if (c->ptr && c->host_size == 8) {
            g_recoff[i] = rec_off;
            rec_off += RIG_REC;
        } else {
            g_recoff[i] = 0xffffffffu;
        }
    }
    g_rec_bytes = rec_off;

    /* Entity-list walk (docs/108): the DS cell table only reaches the list HEAD (0x53c70) and TAIL
     * (0x53c74), so every mid-list entity -- the one that dies, the one that gets hurt, the one whose
     * EXP is awarded -- was invisible to the state oracle. The +0x13f "next" links are followed here
     * and up to RIG_ENTS blocks are dumped per sample, in list order, so both sides can be compared
     * slot by slot. */
    g_ent_base = g_rec_bytes;
    g_rec_bytes += RIG_ENTS * RIG_REC;

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
    for (int k = 0; k < RIG_ENTS; k++)
        fprintf(g_meta, ",{\"ds\":%u,\"name\":\"ent%02d\",\"guest_size\":0,\"host_size\":%d,"
                       "\"ptr\":2,\"offset\":%u,\"rec_offset\":%u,\"rec_bytes\":%d}",
                0x53c70u, k, RIG_REC, g_ent_base + k * RIG_REC, g_ent_base + k * RIG_REC, RIG_REC);
    fprintf(g_meta, "]}\n");
    fflush(g_meta);
}

/* One record per cadence tick. `frame` is the port's presented-frame counter, `audio_bytes` the
 * PCM bytes handed to the audio device (the audio oracle's window bound); `screen_hash` stays zero
 * until the media oracle fills it in. */
void rig_poll(long t_ms0, unsigned long long frame, unsigned long long audio_bytes) {
    /* Cadence is kept on the raw clock, the stamp on the shifted one: an anchor can move the shifted
     * clock backwards by tens of seconds, and comparing the stamp against the cadence deadline then
     * silences the dump until the raw clock catches up (it stopped the dump for the rest of a run). */
    long t_ms = (long)(t_ms0 + g_t_shift_ms);
    if (!g_bin || t_ms0 < g_next_raw_ms) return;
    g_next_raw_ms = t_ms0 + g_period_ms;

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
        if (c->ptr && c->host_size == 8) {
            /* Pointer cells are dumped HEAP-RELATIVE (docs/107). The port's own model already keeps
             * heap pointers as uint32 offsets from g_heap_base (f.h:418), but these globals are host
             * pointers, so the raw 8 bytes changed on every launch (macOS ASLR) -- 74,947 cell-samples
             * differed between two runs of one drive at t=16 ms, which made cross-run and cross-side
             * record comparison useless. The low 4 bytes now hold the arena offset (the same shape the
             * shipped DS holds, e.g. DAT_00051c27c = 0x51c27c); cells that point outside the arena
             * (static tables) read 0 and are identified by their record block instead. */
            const uint8_t *obj = *(const uint8_t *const *)c->host;
            unsigned char *dst = p + g_off[i];
            memset(dst, 0, 8);
            if (obj) {
                /* the port's own convention (f.h:431-435): a signed 32-bit arena-relative offset, so
                 * objects outside the arena keep a stable, comparable value too */
                int32_t off32 = (int32_t)(obj - (const uint8_t *)g_heap_base);
                memcpy(dst, &off32, 4);
            }
            if (g_recoff[i] != 0xffffffffu && obj)
                memcpy(p + g_recoff[i], obj, RIG_REC);   /* stays zero = "null pointer" */
        }
    }
    /* walk the entity list from the head the same way fn_012616's sweep does */
    {
        uint8_t *e = g_dat_53c70;
        for (int k = 0; k < RIG_ENTS && e; k++) {
            memcpy(p + g_ent_base + k * RIG_REC, e, RIG_REC);
            e = *(uint8_t **)(e + 0x13f);
        }
    }
    fwrite(p, g_rec_bytes, 1, g_bin);
    free(p);
    g_tick++;
    if ((g_tick & 15) == 0) fflush(g_bin);
}

/* Windows for the diff tool: named events (scripted keys, scene changes) stamped on the same clock
 * as the records. */
void rig_marker(long t_ms_in, const char *fmt, ...) {
    long t_ms = (long)(t_ms_in + g_t_shift_ms);
    if (!g_marks) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fprintf(g_marks, "{\"tick\":%u,\"t_ms\":%ld,\"what\":\"%s\"}\n", g_tick, t_ms, buf);
    fflush(g_marks);
}

/* ---- per-frame screen signature -----------------------------------------------------------
 * The comparable object is what each side *shows*: the original's raw frame (BGRX32 from the
 * emulator) and the port's composited pixel buffer. A fixed 40x30 mean-luminance grid per frame lets
 * the screen oracle score every frame of a route (1.2 KB/frame) instead of shipping 128 MB of
 * bitmaps; whole frames are written as separate witness files only when the picture changes.
 *
 * The byte offsets are explicit arguments because the two sides do not agree on channel order in
 * memory (the port's RGBA8888 word is little-endian, so its bytes are A,B,G,R) and the channel-order
 * mistakes in docs/70 and docs/79 must show up as "a different permutation scores better" rather
 * than as an unexplained difference. */
#define RIG_GW 40
#define RIG_GH 30

static FILE *g_frames;
static char g_frames_dir[1024];
static int g_witness, g_witness_every, g_last_sig_changed;
static unsigned char g_prev_sig[RIG_GW * RIG_GH];

void rig_screen_init(const char *dir, int witness_every) {
    char path[1024];
    g_witness_every = witness_every;
    if (!dir) return;
    snprintf(g_frames_dir, sizeof g_frames_dir, "%s", dir);
    mkdir(dir, 0777);      /* rig_screen_init runs before rig_init, which also mkdirs */
    snprintf(path, sizeof path, "%s/frames.bin", dir);
    g_frames = fopen(path, "wb");
    fprintf(stderr, "rig_sink: frames %s -> %p\n", path, (void *)g_frames);
    if (!g_frames) return;
    uint32_t hdr[3] = {0x5247464eu /* NGFR */, 1u, RIG_GW * RIG_GH};
    fwrite(hdr, sizeof hdr, 1, g_frames);
}

void rig_screen_frame(long t_ms_in, unsigned long long frame, const void *pixels, int w, int h,
                      int ri, int gi, int bi) {
    if (!g_frames || !pixels || w <= 0 || h <= 0) return;
    const unsigned char *px0 = (const unsigned char *)pixels;
    unsigned char sig[RIG_GW * RIG_GH];
    for (int gy = 0; gy < RIG_GH; gy++) {
        int y = (int)((long long)gy * h / RIG_GH);
        for (int gx = 0; gx < RIG_GW; gx++) {
            int x = (int)((long long)gx * w / RIG_GW);
            const unsigned char *px = px0 + 4 * ((size_t)y * w + x);
            int r = px[ri], g = px[gi], b = px[bi];
            sig[gy * RIG_GW + gx] = (unsigned char)((r * 77 + g * 150 + b * 29) >> 8);
        }
    }
    g_last_sig_changed = memcmp(sig, g_prev_sig, sizeof sig) != 0;
    if (g_last_sig_changed) memcpy(g_prev_sig, sig, sizeof sig);
    /* 32 bytes, no compiler padding: field widths chosen so every member is naturally aligned.
     * scripts/rig/{capture_original,diff_screen}.py read this exact layout. */
    struct { int64_t t_ms; uint64_t frame; int32_t w, height, changed, pad; } rh =
        { (long long)(t_ms_in + g_t_shift_ms), frame, w, h, g_last_sig_changed, 0 };
    fwrite(&rh, sizeof rh, 1, g_frames);
    fwrite(sig, sizeof sig, 1, g_frames);
    fflush(g_frames);

    /* witness frames: bounded, only on change, so a route stays small */
    if (g_witness_every && g_last_sig_changed &&
        (g_witness++ % g_witness_every) == 0) {
        char sub[1024], path[1200];
        snprintf(sub, sizeof sub, "%s/frames", g_frames_dir);
        mkdir(sub, 0777);                                   /* EEXIST is fine */
        snprintf(path, sizeof path, "%s/%06llu_%dx%d.bin", sub, frame, w, h);
        FILE *f = fopen(path, "wb");
        if (f) { fwrite(px0, 4, (size_t)w * h, f); fclose(f); }
    }
}

/* ---- PCM witness --------------------------------------------------------------------------------
 * The container exposes no audio capture (0.85.1 openapi has capture/video/* only), so the port's own
 * PCM path is the witness: the bytes the sys layer hands the SDL stream are tee'd to a WAV. The audio
 * oracle compares onsets/presence/which slot by window, never waveform identity -- the original's
 * side of that comparison comes from DS activity (scripts/rig/diff_activity.py). */
static FILE *g_pcm;
static uint64_t g_pcm_bytes;

static void put32(unsigned char *p, unsigned v) {
    p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24);
}
static void put16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }

void rig_pcm_open(const char *dir) {
    char path[1024];
    if (!dir) return;
    mkdir(dir, 0777);      /* rig_pcm_open runs before rig_init, which also mkdirs */
    snprintf(path, sizeof path, "%s/pcm.wav", dir);
    g_pcm = fopen(path, "wb");
    if (!g_pcm) return;
    /* 44-byte WAV header (sizes fixed up in rig_close). 22050 Hz mono 16-bit is what the board path
     * produces (docs/34); diff_audio.py re-derives the real rate from the port's own record timeline
     * rather than trusting this number. */
    unsigned char h[44];
    memcpy(h, "RIFF", 4);   put32(h + 4, 0x7fffffff); memcpy(h + 8, "WAVEfmt ", 8);
    put32(h + 16, 16);     put16(h + 20, 1);          put16(h + 22, 1);
    put32(h + 24, 22050);  put32(h + 28, 22050 * 2);  put16(h + 32, 2);   put16(h + 34, 16);
    memcpy(h + 36, "data", 4); put32(h + 40, 0);
    fwrite(h, 44, 1, g_pcm);
}

void rig_pcm_write(const void *bytes, size_t n) {
    if (!g_pcm || !bytes) return;
    fwrite(bytes, 1, n, g_pcm);
    g_pcm_bytes += n;
}

/* index=-1 marks a stop. `lba`/`sectors` are the disc extent the guest asked for, which is what the
 * shipped MSCU command carries, so the schedule can be compared against the original's track list. */
void rig_cdda_marker(long t_ms_in, long index, uint32_t lba, uint32_t sectors) {
    if (!g_cdda_idx) return;
    fprintf(g_cdda_idx, "{\"t_ms\":%lld,\"event\":\"%s\",\"index\":%ld,\"lba\":%u,\"sectors\":%u,"
                        "\"bytes\":%llu}\n", (long long)(t_ms_in + g_t_shift_ms),
            index < 0 ? "stop" : "play", index, lba, sectors,
            (unsigned long long)g_cdda_bytes);
    fflush(g_cdda_idx);
    g_cdda_next_ms = t_ms_in + g_period_ms;
}

/* One CD sector (2352 B = 588 stereo frames = 1/75 s of audio) as it was handed to the device. */
void rig_cdda_sample(long t_ms_in, const void *bytes, unsigned n) {
    if (!g_cdda || !bytes) return;
    fwrite(bytes, 1, n, g_cdda);
    g_cdda_bytes += n;
    if (g_cdda_idx && t_ms_in >= g_cdda_next_ms) {
        fprintf(g_cdda_idx, "{\"t_ms\":%lld,\"bytes\":%llu}\n",
                (long long)(t_ms_in + g_t_shift_ms), (unsigned long long)g_cdda_bytes);
        g_cdda_next_ms = t_ms_in + g_period_ms;
    }
}

void rig_close(void) {
    if (g_cdda_idx) { fclose(g_cdda_idx); g_cdda_idx = NULL; }
    if (g_cdda) { fclose(g_cdda); g_cdda = NULL; }
    if (g_pcm) {
        uint32_t dsz = (uint32_t)(g_pcm_bytes < 0x7fffffff ? g_pcm_bytes : 0x7fffffff);
        uint32_t riff = dsz + 36;
        fseek(g_pcm, 4, SEEK_SET);  fwrite(&riff, 4, 1, g_pcm);
        fseek(g_pcm, 40, SEEK_SET); fwrite(&dsz, 4, 1, g_pcm);
        fclose(g_pcm); g_pcm = NULL;
    }
    if (g_bin) fflush(g_bin);
}

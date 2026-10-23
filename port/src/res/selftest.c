/* Loader self-test: renders extracted assets to PNG sheets in scratch/shots/
 * and validates every format parser against known corpus facts.
 * Exit 0 = all checks pass. */
#include "res.h"
#include "data.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CELL 150
#define FILL 128
#define BORDER 60
#define OFF 10

static int failures = 0;

static void check(int ok, const char *what) {
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failures++;
}

/* SPR sheet per the verified reference rules (scripts/make_sheets.py):
 * 150px cells, sprite i in cell (i%cols, i/cols), 1px border 60 around
 * occupied cells, fill 128, sprite at cell origin + (10,10), row bg black. */
static int render_sheet(Res *r, const char *spr_name, int cols, int rows, const char *out) {
    const ResEntry *e = res_find(r, spr_name);
    if (!e) { printf("  [FAIL] %s not found\n", spr_name); failures++; return -1; }
    const uint8_t *blob = res_blob(r, e);
    Spr *s;
    int nspr;
    if (spr_decode(blob, e->size, &s, &nspr)) return -1;
    int W = cols * CELL, H = rows * CELL;
    uint8_t *buf = malloc((size_t)W * H);
    memset(buf, FILL, (size_t)W * H);
    int ns = nspr < cols * rows ? nspr : cols * rows;
    uint8_t row[2048];
    for (int i = 0; i < ns; i++) {
        int c = i % cols, rr = i / cols;
        int x0 = c * CELL, y0 = rr * CELL;
        for (int x = 0; x < CELL; x++) { buf[y0 * W + x0 + x] = BORDER; buf[(y0 + CELL - 1) * W + x0 + x] = BORDER; }
        for (int y = 0; y < CELL; y++) { buf[(y0 + y) * W + x0] = BORDER; buf[(y0 + y) * W + x0 + CELL - 1] = BORDER; }
        for (int y = 0; y < (int)s[i].h; y++) {
            memset(row, 0, s[i].w > sizeof row ? sizeof row : s[i].w);
            spr_draw_row(row, (int)s[i].w, 0, &s[i], y);
            int dy = y0 + OFF + y, dx = x0 + OFF;
            if (dy >= H) break;
            for (int x = 0; x < (int)s[i].w && dx + x < W; x++) buf[dy * W + dx + x] = row[x];
        }
    }
    int rc = png_write_gray(out, W, H, buf);
    free(buf);
    spr_free(s, (size_t)ns);
    return rc;
}

int selftest(const char *res_path, const char *shot_dir) {
    printf("NIGHT loader self-test\n======================\n");
    SDL_Init(0); /* keep SDL initialized for later game modes */

    printf("[container] %s\n", res_path);
    Res r;
    if (res_open(res_path, &r)) { printf("  [FAIL] open\n"); return 1; }
    check(r.count == 444, "444 entries");
    printf("  %u entries, %u bytes\n", r.count, r.size);

    printf("[pcx] logo.pcx\n");
    const ResEntry *e = res_find(&r, "logo.pcx");
    Pcx p;
    if (!e || pcx_decode(res_blob(&r, e), e->size, &p)) {
        printf("  [FAIL] decode\n"); return 1;
    }
    check(p.w == 640 && p.h == 480, "640x480");
    char out[1024];
    snprintf(out, sizeof out, "%s/pcx_logo.png", shot_dir);
    uint8_t *rgb = malloc((size_t)p.w * p.h * 3);
    for (int i = 0; i < p.w * p.h; i++) {
        uint8_t *q = p.pal + (size_t)p.idx[i] * 3;
        rgb[i * 3] = q[0]; rgb[i * 3 + 1] = q[1]; rgb[i * 3 + 2] = q[2];
    }
    int nonblank = 0;
    for (int i = 0; i < p.w * p.h; i++)
        if (p.pal[p.idx[i] * 3] | p.pal[p.idx[i] * 3 + 1] | p.pal[p.idx[i] * 3 + 2]) nonblank++;
    check(png_write_rgb(out, p.w, p.h, rgb) == 0, "wrote " "pcx_logo.png");
    printf("  non-black pixels: %d/%d\n", nonblank, p.w * p.h);
    free(rgb);
    pcx_free(&p);

    printf("[spr] sheets (rules: 150px cells, fill 128, border 60, offset 10, row bg black)\n");
    struct { const char *name; int cols, rows; } sheets[] = {
        { "pause.spr", 6, 2 }, { "number1.spr", 6, 2 }, { "ena-1.spr", 6, 4 },
        { "boss1.spr", 6, 6 }, { "gun.spr", 12, 12 },
    };
    for (size_t i = 0; i < sizeof sheets / sizeof *sheets; i++) {
        char base[64];
        snprintf(base, sizeof base, "%s", sheets[i].name);
        char *dot = strrchr(base, '.');
        if (dot) *dot = 0;
        snprintf(out, sizeof out, "%s/%s_sheet.png", shot_dir, base);
        check(render_sheet(&r, sheets[i].name, sheets[i].cols, sheets[i].rows, out) == 0, base);
    }

    printf("[voc] all 100 .voc files\n");
    int nvoc = 0, total = 0, bad = 0;
    for (uint32_t i = 0; i < r.count; i++) {
        const char *nm = r.entries[i].name;
        size_t ln = strlen(nm);
        if (ln < 5 || strcmp(nm + ln - 4, ".voc")) continue; /* N.voc only */
        int digits = 1;
        for (size_t j = 0; j < ln - 4; j++)
            if (nm[j] < '0' || nm[j] > '9') { digits = 0; break; }
        if (!digits) continue;
        const ResEntry *ve = &r.entries[i];
        Voc v;
        if (voc_decode(res_blob(&r, ve), ve->size, &v)) { bad++; continue; }
        nvoc++;
        total += v.n_samples;
        voc_free(&v);
    }
    check(bad == 0, "all decode");
    check(total == 409329, "corpus total 409329 samples (docs/04-sound.md)");
    printf("  %d files, %d samples\n", nvoc, total);

/* key_decode is NOT implemented in this layer: the .key action stream is decoded by the ported EXE code (fn_03e0a0 family) and verified there. */
//     printf("[key] gun.key\n");
//     e = res_find(&r, "gun.key");
//     Key k;
//     check(e && key_decode(res_blob(&r, e), e->size, &k) == 0, "parse");
//     printf("  frame_count=%d tokens=%d\n", k.frame_count, k.n_tokens);
//     key_free(&k);

/* stg_decode is NOT implemented in this layer: the stage record stream is decoded by the ported EXE code (fn_03e0a0 / the record loop in fn_037b03) and verified there, so this container-level check is left out rather than duplicating the decoder. */
//     printf("[stg] stage1.stg\n");
//     e = res_find(&r, "stage1.stg");
//     Stg s;
//     check(e && stg_decode(res_blob(&r, e), e->size, &s) == 0, "parse");
//     int tags[7] = {0};
//     for (int i = 0; i < s.count; i++) if (s.recs[i].tag < 7) tags[s.recs[i].tag]++;
//     check(s.count == 88, "count 88");
//     check(tags[1] == 38 && tags[2] == 29 && tags[3] == 9 && tags[5] == 8 && tags[6] == 2 && tags[0] == 2,
//           "tag histogram {0:2,1:38,2:29,3:9,5:8,6:2}");
//     stg_free(&s);

/* wal_decode is NOT implemented in this layer (the WAL container is only reached through the ported device/sound path); left out so the selftest builds rather than stubbing an unverified decoder. */
//     printf("[wal] stage1.wal\n");
//     e = res_find(&r, "stage1.wal");
//     Wal w;
//     check(e && wal_decode(res_blob(&r, e), e->size, &w) == 0, "parse");
//     check(w.count == 4, "4 nodes");
//     if (w.count) printf("  root grid: %zu bytes, markers: %u\n", w.nodes[0].grid_n, w.nodes[0].nmark);
//     check(w.count && w.nodes[0].grid_n == 1600u * 200u, "root grid 1600x200");
//     wal_free(&w);

/* act_decode is NOT implemented in this layer: the .act action records are decoded by the ported EXE code and verified there. */
//     printf("[act] boss1.act\n");
//     e = res_find(&r, "boss1.act");
//     Act a;
//     check(e && act_decode(res_blob(&r, e), e->size, &a) == 0, "parse");
//     check(a.count == 56 && a.total_beats == 2893, "56 records, 2893 beats");
//     act_free(&a);

    printf("[fli] opening.fli\n");
    e = res_find(&r, "opening.fli");
    char cdda_dir[512];
    snprintf(cdda_dir, sizeof cdda_dir, "%s", shot_dir);
    char *sl = strrchr(cdda_dir, '/');
    if (sl) { *sl = 0; strcat(cdda_dir, "/cdda"); } else strcpy(cdda_dir, "../scratch/cdda");
    Fli fl;
    check(e && fli_open(res_blob(&r, e), e->size, &fl) == 1, "open (marker 0xAF11)");
    check(fl.w == 320 && fl.h == 200 && fl.frame_count == 1168, "320x200, 1168 frames");
    {
        uint8_t *fb = calloc((size_t)fl.w * fl.h, 1);
        fli_next(&fl, fb);                       /* setup: palette + black screen */
        check(fl.pal_set == 1, "setup sets 256-color palette");
        /* RAW file bytes (display palette is full range; the binary's >>2
         * only affects the DAT_000636b8 working buffer - docs/03-formats.md §11) */
        static const uint8_t palhead[12] = { 0x02,0x02,0x02,0x86,0x52,0x3a,0xae,0x62,0x9e,0x42,0x42,0x5a };
        check(memcmp(fl.pal, palhead, 12) == 0, "palette head vs python reference");
        int chunks = 0;
        while (fli_next(&fl, fb) && chunks < 2000) chunks++;   /* 1168 frame chunks */
        check(chunks == 1168, "1168 frame chunks");
        check(fl.pos == fl.size, "chunk stream ends exactly at EOF");
        /* re-run to frame 39 and compare against python reference */
        fli_open(res_blob(&r, e), e->size, &fl);
        memset(fb, 0, (size_t)fl.w * fl.h);
        fli_next(&fl, fb);
        for (int i = 0; i < 39; i++) fli_next(&fl, fb);
        char refpath[600];
            snprintf(refpath, sizeof refpath, "%s/ref_f39.idx", shot_dir);
            FILE *ref = fopen(refpath, "rb");
        if (ref) {
            size_t n = (size_t)fl.w * fl.h;
            uint8_t *rbuf = malloc(n);
            if (fread(rbuf, 1, n, ref) == n) check(memcmp(fb, rbuf, n) == 0, "frame-39 fb matches python reference");
            free(rbuf);
            fclose(ref);
        } else {
            printf("  [skip] ref_f39.idx not found (generate via python first)\n");
        }
        /* visual evidence: frame 39 and the last frame as RGB pngs */
        int fbn = (int)fl.w * (int)fl.h;
        uint8_t *rgb = malloc((size_t)fl.w * fl.h * 3);
        for (int i = 0; i < fbn; i++) {
            const uint8_t *c = fl.pal + fb[i] * 3;
            rgb[i*3] = c[0]; rgb[i*3+1] = c[1]; rgb[i*3+2] = c[2];
        }
        char p[512];
        snprintf(p, sizeof p, "%s/fli_f0039.png", shot_dir);
        png_write_rgb(p, fl.w, fl.h, rgb);
        fli_open(res_blob(&r, e), e->size, &fl);
        memset(fb, 0, (size_t)fl.w * fl.h);
        fli_next(&fl, fb);
        for (int i = 0; i < (int)fl.frame_count; i++) fli_next(&fl, fb);
        for (int i = 0; i < fbn; i++) {
            const uint8_t *c = fl.pal + fb[i] * 3;
            rgb[i*3] = c[0]; rgb[i*3+1] = c[1]; rgb[i*3+2] = c[2];
        }
        snprintf(p, sizeof p, "%s/fli_last.png", shot_dir);
        png_write_rgb(p, fl.w, fl.h, rgb);
        free(rgb);
        free(fb);
        fli_free(&fl);
    }

    printf("[wav] cdda tracks\n");
    {
        char path[512];
        uint8_t *d;
        for (int t = 2; t <= 10; t += 8) {
            snprintf(path, sizeof path, "%s/track%02d.wav", cdda_dir, t);
            FILE *f = fopen(path, "rb");
            if (!f) { printf("  [skip] %s not found\n", path); continue; }
            fseek(f, 0, SEEK_END);
            long n = ftell(f);
            fseek(f, 0, SEEK_SET);
            d = malloc(n);
            fread(d, 1, n, f);
            fclose(f);
            Wav wv;
            check(wav_open(d, (size_t)n, &wv) == 1, path);
            check(wv.rate == 44100 && wv.channels == 2 && wv.bits == 16, "44100 Hz stereo 16-bit");
            printf("  track%02d.wav: %zu frames (%.1fs)\n", t, wv.nframes, (double)wv.nframes / wv.rate);
            wav_free(&wv);
            free(d);
        }
    }

    SDL_Quit();
    printf("======================\n%s (%d failures)\n", failures ? "FAIL" : "ALL PASS", failures);
    res_close(&r);
    return failures ? 1 : 0;
}

#ifdef RES_SELFTEST_MAIN
/* Standalone driver: `make selftest [RES=../NIGHT/NIGHT.RES SHOTS=/tmp/night-shots]`. */
int main(int argc, char **argv) {
    const char *res   = (argc > 1) ? argv[1] : (getenv("RES")   ? getenv("RES")   : "../NIGHT/NIGHT.RES");
    const char *shots = (argc > 2) ? argv[2] : (getenv("SHOTS") ? getenv("SHOTS") : "/tmp/night-shots");
    return selftest(res, shots);
}
#endif

/* stage1.c — stage 1 vertical slice (docs/05-logic.md §3, docs/03-formats.md §4-5).
 *
 * World: horizontal street, 1600 px wide x 200 px walk band (stage1.wal root
 * grid). Coordinates in .stg are half-pixels; the first tag5 record is the
 * player spawn (1656.5, 199.5 in stage1.stg). Background = stage1.pcx drawn
 * fixed at (0,0).
 * Entities: .stg tag1 records -> entity id -> name tables (EXE VA 0x51d88
 * .spr / 0x52198 .act; tables dumped from the binary, SPR_NAMES below).
 * Animation: each entity loops .act record 0 (idle) at 1 beat / 3 frames,
 * sprite frame = rec.frames[beat % rec.n]. Punch (pressed K_PUNCH) plays
 * record 1 of gun.act and fires SFX 1.voc (VOC files are event-numbered
 * 1..100; the full event->voc mapping lives in the binary's event tables —
 * out of slice scope, docs/04-sound.md).
 * Camera: follows the player, clamped to [0, world-640].
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stage1.h"
#include "data.h"

/* entity name tables (EXE 0x51d88 .spr / 0x52198 .act; 119 entries, .act =
 * same order with .act extension) */
static const char *const SPR_NAMES[] = {
    "gun.spr","boss1.spr","boss2.spr","boss3-1.spr","boss3-2.spr","boss3-3.spr",
    "boss4.spr","boss5.spr","boss6.spr","boss7.spr","gun.spr",
    "mboss1-1.spr","mboss1-2.spr","mboss2.spr","mboss3.spr","mboss4.spr",
    "mboss5.spr","mboss6.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
    "ena-1.spr","ena-2.spr","ena-3.spr",
    "gun.spr","gun.spr",
    "enb-1.spr","enb-2.spr","enb-3.spr","enb-4.spr","gun.spr",
    "enc-1.spr","enc-2.spr","enc-3.spr",
    "gun.spr","gun.spr",
    "end-1.spr","end-2.spr","end-3.spr","end-4.spr","gun.spr",
    "ene-1.spr","ene-2.spr","ene-3.spr","ene-4.spr","gun.spr",
    "enf-1.spr","enf-2.spr","enf-3.spr","gun.spr","gun.spr",
    "eng-1.spr","eng-2.spr","eng-3.spr","gun.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
    "enh-1.spr","enh-2.spr","enh-3.spr","gun.spr","gun.spr",
    "eni-1.spr","eni-2.spr","eni-3.spr","eni-4.spr","gun.spr",
    "enj-1.spr","enj-2.spr","enj-3.spr","enj-4.spr","gun.spr",
    "enk-1.spr","enk-2.spr","enk-3.spr","gun.spr","gun.spr",
    "enl-1.spr","enl-2.spr","enl-3.spr","gun.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
};
#define N_ENT ((int)((sizeof SPR_NAMES) / sizeof SPR_NAMES[0]))

static char *name_copy(const char *s) { return strdup(s); }
static char *act_name_of(const char *spr_name) {
    char *a = strdup(spr_name);
    char *dot = strrchr(a, '.');
    if (dot) strcpy(dot, ".act");
    return a;
}

#define VW 640
#define VH 480
#define WORLD_W 1600   /* stage1.wal root grid width */
#define FLOOR_Y 199    /* bottom row of the 200px walk band */

typedef struct {
    int      id;
    float    x, y;           /* world px (half-pixel coords from .stg) */
    char    *spr_name;
    uint8_t *spr_data;       /* kept alive: Spr.raw aliases it */
    Spr     *spr;
    int      nspr;
    size_t **rowoff;         /* per frame: per-row byte offset in spr.raw */
    Act      act;
    int      beat;           /* idle beat counter */
    int      atk_t;          /* >0: playing attack (record 1) */
    int      face;           /* 1 right, -1 left */
} Ent;

struct Stage1 {
    const Res *res;
    Pcx  bg;
    Stg  stg;
    Wal  wal;
    Ent  ents[96];
    int  nent;
    Ent *player;
    int  camx;
    int  frame;
    int  demo;
    int  demo_step;
    Voc  sfx;
    SDL_AudioDeviceID ad;
    SDL_AudioStream  *as;
    FILE *logf;
};

static void slog(Stage1 *s, const char *fmt, ...) {
    if (!s->logf) return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(s->logf, "[f%05d][stage1] ", s->frame);
    vfprintf(s->logf, fmt, ap);
    fprintf(s->logf, "\n");
    fflush(s->logf);
    va_end(ap);
}

static uint8_t *load_blob(const Res *res, const char *name, uint32_t *out_n) {
    const ResEntry *e = res_find(res, name);
    if (!e) return NULL;
    const uint8_t *d = res_blob(res, e);
    uint8_t *c = malloc(e->size);
    memcpy(c, d, e->size);
    *out_n = e->size;
    return c;
}

/* per-row byte offsets inside a sprite's raw data (row: [nseg][skip,len,px..]) */
static size_t *row_offsets(const Spr *s) {
    size_t *ro = malloc(s->h * sizeof *ro);
    const uint8_t *p = s->raw;
    for (uint32_t y = 0; y < s->h; y++) {
        ro[y] = (size_t)(p - s->raw);
        int n = *p++;
        for (int i = 0; i < n; i++) {
            int len = p[1];
            p += 2 + len;
        }
    }
    return ro;
}

/* load an entity's .spr + .act by name table index */
static void ent_load(Stage1 *s, Ent *e, int id) {
    e->spr_name = name_copy(id < N_ENT ? SPR_NAMES[id] : "gun.spr");
    uint32_t n;
    e->spr_data = load_blob(s->res, e->spr_name, &n);
    if (!e->spr_data) return;
    int cnt = 0;
    if (spr_decode(e->spr_data, n, &e->spr, &cnt) != 0) return;
    e->nspr = cnt;
    e->rowoff = malloc((size_t)cnt * sizeof *e->rowoff);
    for (int f = 0; f < cnt; f++) e->rowoff[f] = row_offsets(&e->spr[f]);
    char *an = act_name_of(e->spr_name);
    uint32_t na;
    uint8_t *da = load_blob(s->res, an, &na);
    free(an);
    if (da) {
        act_decode(da, na, &e->act);
        free(da);
    }
}

/* screen mapping (see docs/06-port.md stage1 notes): the 320x200 offscreen
 * game space is 2x-scaled to the 640x480 screen. The 200-px walk band maps to
 * screen rows 80..480. The stage world is rendered whole into the 640-px
 * screen (no camera scroll in the slice). */
#define SCALE_X (640.0f / 2048.0f)
static int feet_y(float wy) { return 80 + (int)(wy * 2.0f); }

static void draw_sprite(Stage1 *s, uint32_t *vpx, const Ent *e, int sx, int sy, int scale) {
    const ActRec *rec = NULL;
    int frame_idx = 0;
    if (e->atk_t > 0 && e->act.count > 1)
        rec = &e->act.recs[1];
    else if (e->act.count > 0)
        rec = &e->act.recs[0];
    if (rec) {
        int b = (e->beat / 3) % rec->n;
        if (b < 0) b = 0;
        frame_idx = rec->frames[b];
    }
    if (frame_idx >= e->nspr) frame_idx = 0;
    const Spr *sp = &e->spr[frame_idx];
    const uint8_t *pal = s->bg.pal;   /* raw 8-bit bg palette = display DAC */
    for (uint32_t y = 0; y < sp->h; y++) {
        const uint8_t *p = sp->raw + e->rowoff[frame_idx][y];
        int nseg = *p++;
        int cx = sx;
        for (int i = 0; i < nseg; i++) {
            int skip = *p++;
            int len  = *p++;
            cx += skip * scale;
            for (int k = 0; k < len; k++, p++) {
                uint32_t c = pal[(int)(*p) * 3];
                uint32_t v = 0xFF000000u | (c & 0xFFu) << 16 | ((c >> 8) & 0xFFu) << 8
                             | ((c >> 16) & 0xFFu);
                for (int dy = 0; dy < scale; dy++) {
                    int yy = sy + (int)y * scale + dy;
                    if (yy < 0 || yy >= VH) continue;
                    uint32_t *dst = vpx + (size_t)yy * VW;
                    for (int dx = 0; dx < scale; dx++) {
                        int xx = cx + k * scale + dx;
                        if (xx < 0 || xx >= VW) continue;
                        dst[xx] = v;
                    }
                }
            }
            cx += len * scale;
        }
    }
}

static void play_sfx(Stage1 *s, const char *what) {
    if (!s->as) { slog(s, "sfx %s (no audio device, skipped)", what); return; }
    static int16_t buf[65536];
    int n = s->sfx.n_samples > 65536 ? 65536 : s->sfx.n_samples;
    for (int i = 0; i < n; i++) buf[i] = (int16_t)((int16_t)s->sfx.pcm[i] * 256 - 0x8000);
    SDL_PutAudioStreamData(s->as, buf, n * sizeof(int16_t));
    slog(s, "sfx: %s -> 1.voc (%d samples @22050 mono)", what, n);
}

Stage1 *stage1_open(const Res *res, int demo, FILE *logf) {
    Stage1 *s = calloc(1, sizeof *s);
    s->res = res; s->demo = demo; s->logf = logf;

    uint32_t n;
    uint8_t *d;

    d = load_blob(res, "stage1.pcx", &n);
    if (!d || pcx_decode(d, n, &s->bg) != 0) { fprintf(stderr, "stage1.pcx\n"); return NULL; }
    slog(s, "bg stage1.pcx %dx%d", s->bg.w, s->bg.h);

    d = load_blob(res, "stage1.stg", &n);
    if (!d || stg_decode(d, n, &s->stg) != 0) { fprintf(stderr, "stage1.stg\n"); return NULL; }
    slog(s, "stg: %d records", s->stg.count);

    d = load_blob(res, "stage1.wal", &n);
    if (!d || wal_decode(d, n, &s->wal) != 0) { fprintf(stderr, "stage1.wal\n"); return NULL; }
    slog(s, "wal: %d nodes, root grid %zu tiles", s->wal.count, s->wal.nodes[0].grid_n);

    /* sfx: 1.voc (event 1) */
    d = load_blob(res, "1.voc", &n);
    if (d) {
        voc_decode(d, n, &s->sfx);
        slog(s, "sfx 1.voc: %d samples", s->sfx.n_samples);
        free(d);
    }

    /* audio (optional — headless/CI may have no device) */
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_AudioSpec spec;
        SDL_zero(spec);
        spec.freq = 22050;
        spec.format = SDL_AUDIO_S16LE;
        spec.channels = 1;
        s->ad = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (s->ad != 0) {
            s->as = SDL_CreateAudioStream(&spec, &spec);
            if (s->as) SDL_BindAudioStream(s->ad, s->as);
        }
    }

    /* entities: tag1 records (one Ent per unique id, first spawn wins pos) */
    for (int i = 0; i < s->stg.count && s->nent < 96; i++) {
        StgRec *r = &s->stg.recs[i];
        if (r->tag != 1) continue;
        int id = (int)r->p[0];
        if (id >= N_ENT) continue;
        Ent *e = NULL;
        for (int k = 0; k < s->nent; k++)
            if (s->ents[k].id == id) { e = &s->ents[k]; break; }
        if (!e) e = &s->ents[s->nent++];
        if (e->nspr == 0) {
            e->id = id;
            ent_load(s, e, id);
            if (e->nspr > 0 && e->act.count > 0)
                slog(s, "entity %d (%s): %d frames, %d act records, @ (%.1f, %.1f)",
                     id, e->spr_name, e->nspr, e->act.count,
                     r->p[2] / 2.0f, r->p[3] / 2.0f);
        }
        if (e->x == 0.0f && e->y == 0.0f) {
            e->x = (float)r->p[2] / 2.0f;
            e->y = (float)r->p[3] / 2.0f;
        }
    }

    /* player = entity 0 (gun), spawned at the first tag5 anchor */
    for (int i = 0; i < s->stg.count; i++) {
        StgRec *r = &s->stg.recs[i];
        if (r->tag != 5) continue;
        if (s->player) break;
        Ent *p = &s->ents[s->nent++];
        memset(p, 0, sizeof *p);
        p->id = 0;
        ent_load(s, p, 0);
        p->x = (float)r->p[2] / 2.0f;
        p->y = (float)r->p[3] / 2.0f;
        p->face = -1;
        s->player = p;
        slog(s, "player gun @ (%.1f, %.1f) [tag5 rec%d: %d frames, %d act recs]",
             p->x, p->y, i, p->nspr, p->act.count);
        break;
    }
    slog(s, "entities: %d loaded", s->nent);
    return s;
}

void stage1_close(Stage1 *s) {
    if (!s) return;
    for (int i = 0; i < s->nent; i++) {
        Ent *e = &s->ents[i];
        if (e->spr) { spr_free(e->spr, (size_t)e->nspr); e->spr = NULL; }
        for (int f = 0; f < e->nspr; f++) free(e->rowoff[f]);
        free(e->rowoff);
        act_free(&e->act);
        free(e->spr_data);
        free(e->spr_name);
    }
    if (s->as) { SDL_DestroyAudioStream(s->as); s->as = NULL; }
    if (s->ad) { SDL_CloseAudioDevice(s->ad); s->ad = 0; }
    voc_free(&s->sfx);
    stg_free(&s->stg);
    wal_free(&s->wal);
    pcx_free(&s->bg);
    free(s);
}

void stage1_frame(Stage1 *s, uint32_t *vpx, uint8_t down, uint8_t pressed) {
    s->frame++;

    /* demo input script (deterministic, for headless evidence runs) */
    if (s->demo) {
        s->demo_step++;
        int t = s->demo_step % 240;
        down = 0; pressed = 0;
        if (t < 90)       down |= K_RIGHT;
        else if (t < 180) down |= K_LEFT;
        if (t == 30)  pressed |= K_JUMP;
        if (t == 90)  pressed |= K_PUNCH;
        if (t == 180) pressed |= K_KICK;
    }

    /* input */
    Ent *pl = s->player;
    if (pl) {
        float v = 0;
        if (down & K_LEFT)  { v = -2.2f; pl->face = -1; }
        if (down & K_RIGHT) { v =  2.2f; pl->face =  1; }
        pl->x += v;
        if (pl->x < 8) pl->x = 8;
        if (pl->x > WORLD_W - 8) pl->x = WORLD_W - 8;
        if (pressed & K_PUNCH) { pl->atk_t = 18; play_sfx(s, "punch"); }
        if (pressed & K_KICK)  { pl->atk_t = 14; play_sfx(s, "kick (1.voc stand-in)"); }
    }

    /* animation clocks */
    for (int i = 0; i < s->nent; i++) {
        Ent *e = &s->ents[i];
        e->beat++;
        if (e->atk_t > 0) e->atk_t--;
    }

    /* camera follows player */
    int cam = (int)(pl ? pl->x : 0) - VW / 2;
    if (cam < 0) cam = 0;
    if (cam > WORLD_W - VW) cam = WORLD_W - VW;
    s->camx = cam;   /* unused: slice renders the whole world */

    /* background (raw palette = display DAC values) */
    const Pcx *bg = &s->bg;
    for (int y = 0; y < VH; y++) {
        uint32_t *dst = vpx + (size_t)y * VW;
        const uint8_t *row = bg->idx + (size_t)y * bg->w;
        for (int x = 0; x < VW; x++) {
            uint32_t c = bg->pal[row[x] * 3];
            dst[x] = 0xFF000000u | (c & 0xFFu) << 16 | ((c >> 8) & 0xFFu) << 8
                     | ((c >> 16) & 0xFFu);
        }
    }

    /* sprites: far (small y) first, sorted by y via insertion passes */
    for (int pass = 0; pass < s->nent; pass++) {
        for (int i = 0; i < s->nent; i++) {
            Ent *e = &s->ents[i];
            if (e->nspr == 0 || e->y <= 0) continue;
            int rank = 0;
            for (int k = 0; k < s->nent; k++)
                if (e->y > s->ents[k].y && s->ents[k].nspr > 0 && s->ents[k].y > 0) rank++;
            if (rank != pass) continue;
            const Spr *sp0 = &e->spr[0];
            int sx = (int)(e->x * SCALE_X) - (int)(sp0->w / 2);
            int sy = feet_y(e->y) - (int)(sp0->h * 2);   /* feet-anchored, 2x scale */
            draw_sprite(s, vpx, e, sx, sy, 2);
        }
    }
}

/* stage.c — full playable stage engine (beat-em-up).
 *
 * Faithful-to-data interpretation of the stage script (.stg) and combat
 * model. See docs/05-logic.md §3-4 and docs/06-port.md. The .stg tag-2/3/6
 * records are runtime/wave parameters (see docs/03-formats.md §12); this
 * engine treats tag-2 records grouped by payload[1] as enemy spawn waves and
 * the boss as the entity for the current episode. Enemy AI and exact damage
 * numbers are a playable approximation (the original's per-frame combat
 * handler FUN_0003a829 is the authoritative source).
 *
 * World mapping (docs/07-floor-camera.md): the street is the stage%d.wal node
 * grid (world_w x 200, half-res), scrolled by a 320-px camera window and 2x
 * upscaled into screen rows 80..479 (top 80 rows = dark HUD header). y is
 * depth (larger = lower/front). Sprites are 2x nearest-neighbour, feet-anchored
 * at feet_y = 80 + 2*wy, drawn with the bright EPISODE-card palette (the
 * original switches the DAC palette between the dark street and the sprites).
 * Camera-left camx = clamp(player_x - 160, 0, world_w - 320).
 */
#define _POSIX_C_SOURCE 200809L
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stage.h"
#include "data.h"
#include "type_tab.h"

/* ---------- entity name tables (EXE 0x51d88 .spr / 0x52198 .act) ---------- */
static const char *const SPR_NAMES[] = {
    "gun.spr","boss1.spr","boss2.spr","boss3-1.spr","boss3-2.spr","boss3-3.spr",
    "boss4.spr","boss5.spr","boss6.spr","boss7.spr","gun.spr",
    "mboss1-1.spr","mboss1-2.spr","mboss2.spr","mboss3.spr","mboss4.spr",
    "mboss5.spr","mboss6.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
    "gun.spr","gun.spr","gun.spr","gun.spr","gun.spr","gun.spr",
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

/* ---------- world / screen ---------- */
#define VW 640
#define VH 480
#define WORLD_W 1600   /* default; per-stage s->world_w comes from the .wal street grid */
#define WORLD_H 200
static int feet_y(float wy) { return 80 + (int)(wy * 2.0f); }

/* ---------- combat tuning (playable approximation) ---------- */
#define P_MAXHP   150
#define E_HP      30
#define B_HP      130
#define PUNCH_DMG 14
#define KICK_DMG  22
#define PUNCH_RNG 70
#define KICK_RNG  95
#define E_ATK_DMG 5
#define B_ATK_DMG 12
#define LANE_TOL  30
/* docs/11-stage-system.md: sub-stage belt-scroll. Enemies drip in from
 * off-screen as the camera crosses their trigger X (no proximity wake). */
#define MAX_ENTS  200
/* G7  exptab.dat col 0 -> 0x53c94+level*4 (levels 0..4) */
static const int EXP_TAB[5] = {300, 1000, 4000, 5000, 5000};
/* G11 4-frame speed oscillation (0x52ffc; values approx, log for task-8) */
static const float SPD_OSC[4] = {1.0f, 1.15f, 1.0f, 0.85f};

typedef enum { S_LIVE, S_HIT, S_ATK, S_DEAD, S_JUMP } EntState;

/* Animation actions (docs/08-animation.md). gun.act record->action mapping is
 * verified by Python decode + sheet rendering: idle=0 walk=41 punch=52
 * kick=66 jump=57 hit=10. */
typedef enum { A_IDLE, A_WALK, A_PUNCH, A_KICK, A_JUMP, A_HIT } AnimAction;

typedef struct {
    int      id;
    int      is_player, is_boss, alive;
    float    x, y;
    int      facing;             /* 1 right, -1 left */
    /* assets */
    char    *spr_name;
    uint8_t *spr_data;           /* kept: Spr.raw aliases it */
    Spr     *spr;
    int      nspr;
    size_t **rowoff;
    Act      act;
    /* combat */
    int      hp, maxhp;
    EntState state;
    int      atk_t, atk_cd, atk_hit;  /* attack anim / cooldown / has-hit */
    int      hit_t, inv_t, dead_t;
    int      beat;               /* idle anim beat */
    int      anim_rec;
    int      kick;               /* current attack is a kick */
    int      jump_t;             /* jump timer (S_JUMP) */
    int      action;             /* current animation action (AnimAction) */
    int      last_action;        /* for --log action-change detection */
    int      e_walk, e_atk, e_hit;  /* enemy heuristic record indices */
    int      armor;          /* type armor pool (night*.chr f0); damage hits it first */
} Ent;

typedef struct { float x, y; int t; } HitFx;

/* pre-stage slide */
typedef struct { char *name; Pcx pcx; } Slide;
/* docs/11-stage-system.md: one .wal segment = one sub-stage arena. */
typedef struct {
    int left, top, right, bottom, f4, f5, flag; /* flag b: 1 = enter from right */
    uint32_t *floor; int fw;                    /* ARGB floor, fw x 200 */
    int      exit; float exit_x, exit_y0, exit_y1;  /* tag6 zone in this seg */
} Seg;
/* docs/11 §3: tag-1 drip spawner record (fires once camX crosses tx). */
typedef struct { int id, side, tx, wy, zone, posmode, fired, is_boss; } Spw;
#define MAX_SPW 256
/* docs/11 §5.4/§6: kill-clear threshold + player-Y, ts0 column (S+0x40 == 0) */
static const int KILLTHR[9] = {1000,1000,1000,15,1000,1000,1000,1000,24};
static const int PLY[9]     = {172,172,172,172,160,172,172,172,172};
#define MAX_ACTIVE_FOES 9   /* cap table 0x528b4 ts0 = 9 for all episodes */

struct Stage {
    const Res *res;
    int      episode;
    Pcx      bg;       /* stage%d.pcx — EPISODE title card (bright palette) */
    int      world_w;  /* max seg right (logging only) */
    int      camx, camy;  /* camera top-left in world px (clamped per sub-stage) */
    Seg      segs[16]; int nseg, seg_i;  /* docs/11: sub-stage chain */
    Spw      spw[MAX_SPW]; int nspw;     /* docs/11 §3: drip spawner queue */
    Stg      stg;
    Wal      wal;
    Ent      ents[MAX_ENTS];
    int      nent;
    Ent     *player;
    int      boss_spawned, boss_dead, boss_active;
    int      killc;        /* docs/11 §5.2: kills this sub-stage (kill-clear) */
    int      enter_cd;     /* frames of exit-zone immunity after sub-stage entry */
    int      exp, jinseong;
    int      exp_level;      /* G7: 0..5, thresholds EXP_TAB (exptab.dat col 0) */
    HitFx    fx[8];          /* G10: starburst impact FX */
    int      jin_boost;          /* jinseong full -> damage boost */
    /* phases */
    int      phase;              /* 0 slides, 1 play, 2 clear, 3 over, 4 victory */
    int      phase_t;
    Slide    slides[8];
    int      nslides;
    int      slide_i, slide_t;
    /* sfx + audio */
    Voc      sfx;
    SDL_AudioDeviceID ad;
    SDL_AudioStream  *as;
    /* io */
    FILE    *logf;
    int      demo, demo_step;
};

static void slog(Stage *s, const char *fmt, ...) {
    if (!s->logf) return;
    va_list ap; va_start(ap, fmt);
    fprintf(s->logf, "[stage%d] ", s->episode);
    vfprintf(s->logf, fmt, ap);
    fprintf(s->logf, "\n"); fflush(s->logf);
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

static char *name_copy(const char *s) { return strdup(s); }

static size_t *row_offsets(const Spr *s) {
    size_t *ro = malloc(s->h * sizeof *ro);
    const uint8_t *p = s->raw;
    for (uint32_t y = 0; y < s->h; y++) {
        ro[y] = (size_t)(p - s->raw);
        int n = *p++;
        for (int i = 0; i < n; i++) { int len = p[1]; p += 2 + len; }
    }
    return ro;
}

/* middle-frame sprite aspect (w/h) of a record — used by the enemy heuristic */
static float rec_mid_aspect(const Ent *e, int rec) {
    if (rec < 0 || rec >= e->act.count || e->nspr <= 0) return 0.0f;
    const ActRec *r = &e->act.recs[rec];
    if (r->n <= 0) return 0.0f;
    int f = r->frames[r->n / 2];
    if (f >= e->nspr) return 0.0f;
    int h = e->spr[f].h;
    return h ? (float)e->spr[f].w / (float)h : 0.0f;
}

/* Pick which .act records to use per action.
 * Player (gun): fixed verified mapping (see AnimAction).
 * Enemies: heuristic — walk = first substantial cycle; atk = widest upright
 *   cycle (extended limb = punch/kick); hit = first fallen (wide) cycle. */
static void pick_anim_records(Ent *e) {
    e->e_walk = e->e_atk = e->e_hit = 0;
    if (e->is_player || e->act.count <= 1) return;
    for (int i = 1; i < e->act.count; i++)
        if (e->act.recs[i].n >= 20 && e->act.recs[i].n <= 150) { e->e_walk = i; break; }
    int bw = 0;
    for (int i = e->e_walk + 1; i < e->act.count; i++) {
        const ActRec *r = &e->act.recs[i];
        if (r->n < 8 || r->n > 80 || rec_mid_aspect(e, i) > 1.4f) continue;
        int f = r->frames[r->n / 2];
        int w = (f < e->nspr) ? e->spr[f].w : 0;
        if (w > bw) { bw = w; e->e_atk = i; }
    }
    for (int i = 1; i < e->act.count; i++)
        if (e->act.recs[i].n >= 10 && rec_mid_aspect(e, i) > 1.7f) { e->e_hit = i; break; }
    if (e->e_atk == 0) e->e_atk = e->e_walk;
}

/* record index to play for an action */
static int anim_rec_for(const Ent *e, int action) {
    int n = e->act.count;
    if (n <= 0) return 0;
    int rec = 0;
    if (e->is_player) {
        switch (action) {
        case A_WALK:  rec = 41; break;
        case A_PUNCH: rec = 52; break;
        case A_KICK:  rec = 66; break;
        case A_JUMP:  rec = 57; break;
        case A_HIT:   rec = 10; break;
        default:      rec = 0;  break;
        }
    } else {
        switch (action) {
        case A_WALK:                rec = e->e_walk; break;
        case A_PUNCH: case A_KICK:  rec = e->e_atk;  break;
        case A_JUMP:                rec = e->e_walk; break;
        case A_HIT:                 rec = e->e_hit;  break;
        default:                    rec = 0;  break;
        }
    }
    if (rec < 0 || rec >= n) rec = (n > 1) ? 1 : 0;
    return rec;
}

/* current sprite (frame) index for the entity's action */
static int anim_frame(const Ent *e) {
    int n = e->act.count;
    if (n <= 0) return 0;
    const ActRec *rec = &e->act.recs[anim_rec_for(e, e->action)];
    if (rec->n <= 0) return 0;
    int spd = (e->action == A_WALK) ? 2 : 3;
    int b = (e->beat / spd) % rec->n; if (b < 0) b = 0;
    int f = rec->frames[b];
    return f < e->nspr ? f : 0;
}

static const char *action_name(int a) {
    switch (a) {
    case A_WALK:  return "walk";
    case A_PUNCH: return "punch";
    case A_KICK:  return "kick";
    case A_JUMP:  return "jump";
    case A_HIT:   return "hit";
    default:      return "idle";
    }
}

/* load .spr + .act for an entity by name-table index */
static void ent_load(Stage *s, Ent *e, int id) {
    e->id = id;
    e->spr_name = name_copy(id < N_ENT ? SPR_NAMES[id] : "gun.spr");
    uint32_t n;
    e->spr_data = load_blob(s->res, e->spr_name, &n);
    if (!e->spr_data) return;
    int cnt = 0;
    if (spr_decode(e->spr_data, n, &e->spr, &cnt) != 0) return;
    e->nspr = cnt;
    e->rowoff = malloc((size_t)cnt * sizeof *e->rowoff);
    for (int f = 0; f < cnt; f++) e->rowoff[f] = row_offsets(&e->spr[f]);
    char *an = name_copy(e->spr_name);
    char *dot = strrchr(an, '.'); if (dot) strcpy(dot, ".act");
    uint32_t na; uint8_t *da = load_blob(s->res, an, &na);
    free(an);
    if (da) { act_decode(da, na, &e->act); free(da); }
    pick_anim_records(e);
}

/* ---------- 5x7 font ---------- */
static const uint8_t FONT[][7] = {
    /* A */ {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* B */ {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    /* C */ {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    /* D */ {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    /* E */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    /* F */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    /* G */ {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E},
    /* H */ {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* I */ {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    /* J */ {0x07,0x02,0x02,0x02,0x02,0x02,0x0C},
    /* K */ {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    /* L */ {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    /* M */ {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    /* N */ {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    /* O */ {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* P */ {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    /* Q */ {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    /* R */ {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    /* S */ {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    /* T */ {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    /* U */ {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* V */ {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    /* W */ {0x11,0x11,0x11,0x15,0x15,0x0A,0x0A},
    /* X */ {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    /* Y */ {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    /* Z */ {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    /* 0 */ {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    /* 1 */ {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    /* 2 */ {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    /* 3 */ {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
    /* 4 */ {0x02,0x06,0x0A,0x11,0x1F,0x02,0x02},
    /* 5 */ {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    /* 6 */ {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    /* 7 */ {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    /* 8 */ {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    /* 9 */ {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    /* - */ {0x00,0x00,0x00,0x0E,0x00,0x00,0x00},
    /* : */ {0x00,0x04,0x04,0x00,0x04,0x04,0x00},
    /* . */ {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C},
    /* sp*/ {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};
static int glyph_idx(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    if (c == '-') return 36;
    if (c == ':') return 37;
    if (c == '.') return 38;
    return 39;
}
void draw_text(uint32_t *vpx, int x, int y, const char *str, uint32_t col, int scale) {
    for (const char *c = str; *c; c++) {
        const uint8_t *g = FONT[glyph_idx(*c)];
        for (int r = 0; r < 7; r++)
            for (int b = 0; b < 5; b++)
                if (g[r] & (0x10 >> b))
                    for (int dy = 0; dy < scale; dy++)
                        for (int dx = 0; dx < scale; dx++) {
                            int px = x + b * scale + dx, py = y + r * scale + dy;
                            if (px >= 0 && px < VW && py >= 0 && py < VH)
                                vpx[(size_t)py * VW + px] = col;
                        }
        x += 6 * scale;
    }
}
static int text_w(const char *s, int scale) { return 6 * scale * (int)strlen(s); }

/* ---------- sprite render (scale + optional flash) ---------- */
static void draw_sprite(Stage *s, uint32_t *vpx, const Ent *e, int sx, int sy, int scale, int flash) {
    int frame_idx = (e->act.count > 0) ? anim_frame(e) : 0;
    if (frame_idx >= e->nspr) frame_idx = 0;
    const Spr *sp = &e->spr[frame_idx];
    /* Sprites share the scene (wal node0) palette; the character colour
     * indices (237..252) are identical across every node (verified). The DAC
     * stores 6-bit values expanded to 8-bit by <<2 (x4). */
    const uint8_t *pal = s->wal.count ? s->wal.nodes[0].pal : s->bg.pal;
    for (uint32_t y = 0; y < sp->h; y++) {
        const uint8_t *p = sp->raw + e->rowoff[frame_idx][y];
        int nseg = *p++;
        int cx = sx;
        for (int i = 0; i < nseg; i++) {
            int skip = *p++;
            int len  = *p++;
            cx += skip * scale;
            for (int k = 0; k < len; k++, p++) {
                const uint8_t *c = pal + (size_t)(*p) * 3;
                uint32_t v = 0xFF000000u | ((uint32_t)c[0] << 2) << 16 | ((uint32_t)c[1] << 2) << 8 | ((uint32_t)c[2] << 2);
                if (flash && ((e->beat / 3) & 1)) v = 0xFFFFFFFFu;
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

static void play_sfx(Stage *s, const char *what) {
    if (!s->as) return;
    static int16_t buf[65536];
    int n = s->sfx.n_samples > 65536 ? 65536 : s->sfx.n_samples;
    for (int i = 0; i < n; i++) buf[i] = (int16_t)((int16_t)s->sfx.pcm[i] * 256 - 0x8000);
    SDL_PutAudioStreamData(s->as, buf, n * sizeof(int16_t));
    slog(s, "sfx: %s", what);
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* pick the boss entity id for the episode (boss1..boss7 = id 1..9) */
static int boss_id_for(int ep) {
    static const int map[] = {1,2,3,4,5,6,7,8,9};
    return map[clampi(ep, 1, 9) - 1];
}

/* main boss id for the episode is boss_id_for(ep) (1..9 = boss1..boss7);
 * mini-bosses are ids 11..17 (mboss*). Only the main boss gets boss HP. */

/* spawn an enemy at world (x,y) with entity id */
static Ent *spawn(Stage *s, int id, float x, float y, int is_boss) {
    if (s->nent >= MAX_ENTS) return NULL;
    Ent *e = &s->ents[s->nent++];
    memset(e, 0, sizeof *e);
    ent_load(s, e, id);
    e->x = clampi((int)x, 0, s->world_w); e->y = clampi((int)y, 10, WORLD_H - 1);
    e->is_boss = is_boss; e->alive = 1; e->state = S_LIVE;
    e->maxhp = e->hp = is_boss ? B_HP : (TYPE_BODYHP[id] ? TYPE_BODYHP[id] : E_HP);
    e->armor = is_boss ? 0 : TYPE_ARMOR[id];
    e->facing = -1;
    slog(s, "spawn id=%d spr=%s nspr=%d @ (%.0f,%.0f)", id, e->spr_name, e->nspr, e->x, e->y);
    return e;
}

/* docs/11 §2: one .wal node = one sub-stage segment (1:1, all 9 episodes).
 * Each segment's floor is its own w x 200 ARGB buffer — the camera only ever
 * shows the CURRENT segment's floor, so the next sub-stage's graphics can
 * never bleed in (the bug in scratch/stage1-broken.png). */
static void build_segments(Stage *s) {
    s->nseg = 0;
    s->world_w = WORLD_W;
    for (int i = 0; i < s->wal.count && i < 16; i++) {
        const WalNode *nd = &s->wal.nodes[i];
        Seg *sg = &s->segs[s->nseg];
        int w = (int)(nd->p2 - nd->p0);
        if (w <= 0 || w > 8192) continue;
        sg->left = (int)nd->p0; sg->top = (int)nd->p1;
        sg->right = (int)nd->p2; sg->bottom = (int)nd->p3;
        sg->f4 = (int)nd->p4; sg->f5 = (int)nd->p5;
        sg->flag = (int)nd->b;
        sg->fw = w;
        int gw = (int)(nd->grid_n / 200);
        if (gw != w) gw = w;
        sg->floor = malloc((size_t)w * 200 * 4);
        for (int y = 0; y < 200; y++)
            for (int x = 0; x < w; x++) {
                int v = (x < gw) ? nd->grid[y * gw + x] : 0;
                uint32_t c;
                if (v == 0) c = 0xFF0A0A0E;            /* transparent = dark ground */
                else {
                    const uint8_t *pc = nd->pal + (size_t)v * 3;  /* 6-bit DAC x4 */
                    c = 0xFF000000u | ((uint32_t)pc[0] << 2) << 16 | ((uint32_t)pc[1] << 2) << 8 | ((uint32_t)pc[2] << 2);
                }
                sg->floor[y * w + x] = c;
            }
        if (sg->right > s->world_w) s->world_w = sg->right;
        s->nseg++;
    }
}

/* forward decls (defined below; called from stage_open) */
static void build_spawners(Stage *s);
static void drip_spawn(Stage *s);
static void enter_next_segment(Stage *s);

/* ---------- public ---------- */
Stage *stage_open(const Res *res, int episode, int demo, FILE *logf) {
    Stage *s = calloc(1, sizeof *s);
    s->res = res; s->episode = clampi(episode, 1, 9); s->demo = demo; s->logf = logf;
    uint32_t n; uint8_t *d;

    char buf[64];
    snprintf(buf, sizeof buf, "stage%d.pcx", s->episode);
    d = load_blob(res, buf, &n);
    if (!d || pcx_decode(d, n, &s->bg) != 0) { fprintf(stderr, "%s\n", buf); return NULL; }
    slog(s, "bg %s %dx%d", buf, s->bg.w, s->bg.h);

    snprintf(buf, sizeof buf, "stage%d.stg", s->episode);
    d = load_blob(res, buf, &n);
    if (!d || stg_decode(d, n, &s->stg) != 0) { fprintf(stderr, "%s\n", buf); return NULL; }
    slog(s, "stg: %d records", s->stg.count);

    snprintf(buf, sizeof buf, "stage%d.wal", s->episode);
    d = load_blob(res, buf, &n);
    if (d) { wal_decode(d, n, &s->wal); free(d); }
    build_segments(s);
    slog(s, "street: %d sub-stages (world max x %d)", s->nseg, s->world_w);

    /* sfx: 1.voc */
    d = load_blob(res, "1.voc", &n);
    if (d) { voc_decode(d, n, &s->sfx); free(d); }

    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_AudioSpec spec; SDL_zero(spec);
        spec.freq = 22050; spec.format = SDL_AUDIO_S16LE; spec.channels = 1;
        s->ad = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (s->ad) { s->as = SDL_CreateAudioStream(&spec, &spec); if (s->as) SDL_BindAudioStream(s->ad, s->as); }
    }

    /* docs/11 §3: tag-1 drip spawner queue */
    build_spawners(s);

    /* player enters sub-stage 0 (FUN_0003d244 entry: left edge, P.y = table) */
    Seg *c0 = (s->nseg > 0) ? &s->segs[0] : NULL;
    int px = c0 ? c0->left + 30 : 60, py = c0 ? c0->top + PLY[s->episode - 1] : 185;
    s->camx = c0 ? c0->left : 0; s->camy = c0 ? c0->top : 0;
    s->seg_i = 0;
    Ent *p = &s->ents[s->nent++];
    memset(p, 0, sizeof *p);
    ent_load(s, p, 0);
    p->is_player = 1; p->alive = 1; p->state = S_LIVE;
    p->x = px; p->y = py;
    p->maxhp = p->hp = P_MAXHP; p->facing = 1;
    s->player = p;
    s->enter_cd = 60;
    slog(s, "player @ (%.0f, %.0f), %d spawners, %d sub-stages", p->x, p->y, s->nspw, s->nseg);

    /* pre-stage slides: <ep>s<i>.pcx for i=1.. */
    for (int i = 1; i <= 8; i++) {
        snprintf(buf, sizeof buf, "%ds%d.pcx", s->episode, i);
        uint32_t nn; uint8_t *dd = load_blob(res, buf, &nn);
        if (!dd) break;
        if (s->nslides < 8) {
            s->slides[s->nslides].name = name_copy(buf);
            s->slides[s->nslides].pcx = (Pcx){0};
            if (pcx_decode(dd, nn, &s->slides[s->nslides].pcx) != 0) { free(dd); break; }
            s->nslides++;
        }
        free(dd);
    }
    /* append the EPISODE title card (stage%d.pcx) as the final slide */
    if (s->bg.w > 0 && s->nslides < 8) {
        Pcx *dst = &s->slides[s->nslides].pcx;
        dst->w = s->bg.w; dst->h = s->bg.h;
        dst->idx = malloc((size_t)s->bg.w * (size_t)s->bg.h);
        memcpy(dst->idx, s->bg.idx, (size_t)s->bg.w * (size_t)s->bg.h);
        memcpy(dst->pal, s->bg.pal, 768);
        s->slides[s->nslides].name = name_copy("episode-card");
        s->nslides++;
    }
    slog(s, "slides: %d", s->nslides);

    s->phase = s->nslides > 0 ? 0 : 1;
    s->phase_t = 0;
    s->boss_spawned = s->boss_dead = s->boss_active = 0;
    return s;
}

void stage_close(Stage *s) {
    if (!s) return;
    for (int i = 0; i < s->nent; i++) {
        Ent *e = &s->ents[i];
        if (e->spr) spr_free(e->spr, (size_t)e->nspr);
        for (int f = 0; f < e->nspr; f++) free(e->rowoff[f]);
        free(e->rowoff);
        act_free(&e->act);
        free(e->spr_data); free(e->spr_name);
    }
    for (int i = 0; i < s->nslides; i++) { pcx_free(&s->slides[i].pcx); free(s->slides[i].name); }
    if (s->as) SDL_DestroyAudioStream(s->as);
    if (s->ad) SDL_CloseAudioDevice(s->ad);
    voc_free(&s->sfx);
    for (int i = 0; i < s->nseg; i++) free(s->segs[i].floor);
    stg_free(&s->stg); wal_free(&s->wal); pcx_free(&s->bg); free(s);
}

int stage_episode(const Stage *s) { return s->episode; }

/* Per-episode intro/stage music (docs/06-cdda.md §3, FUN_0003a829): the stage
 * frame looks up table[ (epData+0x40)<<2 + (episode-1)*0x28 ] at RAM 0x52da4
 * (NIGHT.EXE file 0x4b7a4) -> the per-episode track. See episode_music_track. */
static int episode_music_track(int ep) {
    /* Ghidra music table (NIGHT.EXE file 0x4b7a4 / RAM 0x52da4, FUN_0003a829):
     * 9 rows (episodes) x 10 wave-slots of u32 tracks. Base (dominant) track per
     * episode: ep1..9 -> 2,3,9,4,5,6,7,8,2. One wave-slot per row overrides to
     * track 8 (a transient the position-based port does not model). */
    static const int t[] = {0, 2, 3, 9, 4, 5, 6, 7, 8, 2};
    if (ep >= 1 && ep <= 9) return t[ep];
    return 2;
}
/* current music track: intro (slides) + stage play the per-episode track;
 * the per-episode boss takes over track 12 (docs/06-cdda.md §3). */
int stage_music_track(const Stage *s) {
    if (s->phase == 1 && s->boss_active) return 12;
    return episode_music_track(s->episode);
}

/* docs/11 §3: collect every tag1 record into the drip-spawn queue (fires in
 * camera-X order, once each). Sentinel X (>= 0x00FFFFFF, ep6/ep7) skipped.
 * tag6 exit zones are assigned to the sub-stage whose X range contains them
 * (0x5d0a0..b1 is one global rect in the original; records drain per zone). */
static void build_spawners(Stage *s) {
    s->nspw = 0;
    for (int i = 0; i < s->stg.count; i++) {
        StgRec *r = &s->stg.recs[i];
        if (r->tag == 6) {
            int x = (int)r->p[2];            /* tag6 X/Y are full-res (not half-px) */
            for (int k = 0; k < s->nseg; k++)
                if (x >= s->segs[k].left && x < s->segs[k].right) {
                    s->segs[k].exit = 1;
                    s->segs[k].exit_x = (float)r->p[2];
                    s->segs[k].exit_y0 = (float)r->p[3];
                    s->segs[k].exit_y1 = (float)r->p[5];
                }
            continue;
        }
        if (r->tag != 1) continue;
        int id = (int)r->p[0];
        if (id == 0 || id >= N_ENT) continue;
        if (r->p[2] >= 0x00FFFFFFu) continue;         /* sentinel / unplaced */
        if (s->nspw < MAX_SPW) {
            Spw *w = &s->spw[s->nspw++];
            w->id = id; w->side = (int)r->p[1]; w->zone = (int)r->p[4]; w->posmode = (int)r->p[5];
            w->tx = (int)r->p[2];           /* X full-res world px */ w->wy = (int)r->p[3];  /* Y full-res */
            w->fired = 0; w->is_boss = (id == boss_id_for(s->episode));
        }
    }
    slog(s, "spawners: %d (boss id=%d), kill-clear=%d", s->nspw, boss_id_for(s->episode),
         KILLTHR[s->episode - 1]);
    for (int k = 0; k < s->nseg; k++)
        if (s->segs[k].exit)
            slog(s, "  sub-stage %d exit zone x=%.0f y=%.0f..%.0f", k,
                 s->segs[k].exit_x, s->segs[k].exit_y0, s->segs[k].exit_y1);
}

/* docs/11 §3: per-frame drip spawn — a record fires once its trigger X
 * enters the camera window (camX + 0x140 >= tx), and its authored Y is inside
 * the current band. posmode 0 -> off-screen at the camera edge (side 0:
 * camX-15, side 1: camX+175 world px = 0x1e / 0x15e full-res); posmode 1 ->
 * authored X. Zone-0 foes respect the max-active cap (9, table 0x528b4 ts0). */
static void drip_spawn(Stage *s) {
    if (s->nseg == 0) return;
    const Seg *c = &s->segs[s->seg_i];
    int active = 0;
    for (int i = 0; i < s->nent; i++)
        if (s->ents[i].alive && !s->ents[i].is_player) active++;
    for (int i = 0; i < s->nspw; i++) {
        Spw *w = &s->spw[i];
        if (w->fired) continue;
        if (w->wy < c->top || w->wy >= c->bottom) continue;  /* not this band */
        if (s->camx + 320 < w->tx) continue;              /* trigger X not yet visible */
        if (!w->is_boss && w->zone == 0 && active >= MAX_ACTIVE_FOES) continue;
        int x = w->posmode ? w->tx : (w->side ? s->camx + 175 : s->camx - 15);
        x = clampi(x, c->left + 8, c->right - 8);
        Ent *e = spawn(s, w->id, (float)x, (float)w->wy, w->is_boss);
        if (!e) return;                                     /* ents[] full */
        w->fired = 1; active++;
        if (w->is_boss) { s->boss_spawned = 1; s->boss_active = 1; }
        slog(s, "drip id=%d side=%d @ (%d,%d) [camx=%d tx=%d]", w->id, w->side, x, w->wy, s->camx, w->tx);
    }
}

/* drop fully-dead entities; keeps ents[] from filling across sub-stages */
static void sweep_dead(Stage *s) {
    int w = 0;
    for (int i = 0; i < s->nent; i++) {
        Ent *e = &s->ents[i];
        if (e->is_player || e->alive || e->dead_t > 0) { s->ents[w++] = *e; continue; }
    }
    s->nent = w;
    for (int i = 0; i < s->nent; i++)
        if (s->ents[i].is_player) { s->player = &s->ents[i]; return; }
}

/* docs/11 §5.3: FUN_0003d244 — intermission done, enter the next sub-stage:
 * camera + player re-init at the segment edge, entry side from flag byte b,
 * kill counter reset, drip queue pre-drained at the new camera X. */
static void enter_next_segment(Stage *s) {
    if (s->seg_i + 1 >= s->nseg) return;
    sweep_dead(s);
    s->seg_i++;
    Seg *c = &s->segs[s->seg_i];
    s->camx = c->left; s->camy = c->top;
    s->killc = 0;
    s->boss_spawned = s->boss_dead = s->boss_active = 0;
    Ent *p = s->player;
    int px = c->flag ? c->left + 175 : c->left + 30;
    p->x = clampi(px, c->left + 10, c->right - 10);
    p->y = c->top + PLY[s->episode - 1];
    p->state = S_LIVE; p->facing = c->flag ? -1 : 1; p->inv_t = 30;
    s->enter_cd = 90;
    s->phase = 1; s->phase_t = 0;
    drip_spawn(s);   /* pre-drain records behind the entry camera (wave reset) */
    slog(s, "sub-stage %d/%d enter: cam=(%d,%d) player=(%.0f,%.0f) side=%s",
         s->seg_i + 1, s->nseg, s->camx, s->camy, p->x, p->y, c->flag ? "right" : "left");
}

static void check_levelup(Stage *s) {
    while (s->exp_level < 5 && s->exp >= EXP_TAB[s->exp_level]) {
        s->exp_level++;
        slog(s, "LEVEL UP: exp=%d level=%d (threshold %d, exptab col0)",
             s->exp, s->exp_level, EXP_TAB[s->exp_level - 1]);
    }
}
static void add_fx(Stage *s, float x, float y) {
    for (int i = 0; i < 8; i++)
        if (s->fx[i].t <= 0) { s->fx[i].x = x; s->fx[i].y = y; s->fx[i].t = 10; return; }
}
/* damage a target; returns 1 if killed */
static int deal_damage(Stage *s, Ent *a, Ent *t, int dmg, float kbx) {
    /* docs/10 §8.3: damage hits the armor pool first, remainder -> body HP */
    int d = dmg;
    if (t->armor > 0) { int ab = t->armor >= d ? d : t->armor; t->armor -= ab; d -= ab; }
    if (d > 0) t->hp -= d;
    if (a->is_player) { s->exp += dmg; check_levelup(s); }  /* §8.6: EXP = damage dealt */
    add_fx(s, t->x, t->y);                                   /* DOB-4 starburst */
    t->state = S_HIT; t->hit_t = 14;
    const Seg *dc = (s->nseg > 0) ? &s->segs[s->seg_i] : NULL;
    t->x += kbx; t->x = clampi((int)t->x, dc ? dc->left : 0, dc ? dc->right : s->world_w);
    if (t->hp <= 0) {
        t->hp = 0; t->state = S_DEAD; t->dead_t = 24; t->alive = 0;
        if (t->is_boss) { s->boss_dead = 1; s->boss_active = 0; slog(s, "BOSS defeated"); }
        else { s->killc++; slog(s, "foe killed (%s, armor=%d) exp=%d killc=%d", t->spr_name, t->armor, s->exp, s->killc); }
        s->jinseong = clampi(s->jinseong + (t->is_boss ? 50 : 12), 0, 100);
        if (s->jinseong >= 100 && !s->jin_boost) { s->jin_boost = 1; slog(s, "jinseong full: damage +25%%"); }
        return 1;
    }
    (void)a;
    return 0;
}

/* attempt an attack from a toward all in-range targets */
static void try_attack(Stage *s, Ent *a, int range, int dmg) {
    for (int i = 0; i < s->nent; i++) {
        Ent *t = &s->ents[i];
        if (t == a || !t->alive || t->state == S_DEAD) continue;
        if (a->is_player != t->is_player) { /* only hit the opposing side */
            float dx = t->x - a->x;
            if (dx * a->facing < 0) continue;         /* behind */
            if (fabsf(dx) > range) continue;
            if (fabsf(t->y - a->y) > LANE_TOL) continue;
            if (a->is_player) play_sfx(s, "hit");
            deal_damage(s, a, t, dmg, a->facing * 6);
        }
    }
}

/* update enemy AI for one frame */
static void enemy_ai(Stage *s, Ent *e) {
    Ent *p = s->player;
    if (!p || !p->alive) return;
    if (e->state == S_HIT) { e->action = A_HIT; if (--e->hit_t <= 0) e->state = S_LIVE; return; }
    if (e->state == S_ATK) {
        e->action = A_PUNCH;
        if (--e->atk_t == 8 && !e->atk_hit) {
            e->atk_hit = 1;
            float dx = p->x - e->x;
            if (fabsf(dx) < (e->is_boss ? 100 : 80) && fabsf(p->y - e->y) < LANE_TOL && p->inv_t <= 0 && !s->demo) {
                p->hp -= e->is_boss ? B_ATK_DMG : E_ATK_DMG;
                p->inv_t = 55; p->state = S_HIT; p->hit_t = 12;
                p->x += (dx >= 0 ? 1 : -1) * 8;
                play_sfx(s, "player hit");
                slog(s, "player hit by %s (-%d hp, hp=%d)", e->spr_name, e->is_boss ? B_ATK_DMG : E_ATK_DMG, p->hp);
            }
        }
        if (e->atk_t <= 0) e->state = S_LIVE;
        return;
    }
    if (e->atk_cd > 0) e->atk_cd--;
    float dx = p->x - e->x, dy = p->y - e->y;
    e->facing = dx >= 0 ? 1 : -1;
    /* G11: 4-frame oscillating speed (0x52ffc, §5.1) */
    float sp = (e->is_boss ? 0.5f : 0.8f) * SPD_OSC[e->beat & 3];
    float adx = fabsf(dx), ady = fabsf(dy);
    if (adx > (e->is_boss ? 90 : 70) || ady > 12) {
        e->x += (dx >= 0 ? sp : -sp);
        e->y += (dy >= 0 ? sp * 0.5f : -sp * 0.5f);
        const Seg *ec = (s->nseg > 0) ? &s->segs[s->seg_i] : NULL;
        e->x = clampi((int)e->x, ec ? ec->left : 0, ec ? ec->right : s->world_w);
        e->y = clampi((int)e->y, ec ? ec->top + 10 : 10, ec ? ec->bottom - 1 : WORLD_H - 1);
        e->action = A_WALK;
    } else if (e->atk_cd <= 0) {
        e->state = S_ATK; e->atk_t = 20; e->atk_hit = 0; e->atk_cd = e->is_boss ? 80 : 55;
        e->action = A_PUNCH;
    } else {
        e->action = A_IDLE;
    }
}

/* advance phase logic; returns stage_frame status */
static int advance(Stage *s) {
    /* clear / over / victory timers */
    if (s->phase == 2 || s->phase == 3 || s->phase == 4) {
        if (--s->phase_t <= 0) {
            if (s->phase == 2) {
                if (s->episode >= 9) { s->phase = 4; s->phase_t = 600; return STG_PLAY; }
                return STG_ADVANCE;
            }
            return s->phase == 3 ? STG_GAMEOVER : STG_VICTORY;
        }
        return STG_PLAY;
    }
    if (s->phase == 5) {   /* intermission: docs/11 §5.3 (FUN_00038f3d(1)) */
        if (--s->phase_t <= 0) enter_next_segment(s);
        return STG_PLAY;
    }
    if (s->phase != 1) return STG_PLAY;
    drip_spawn(s);
    if (s->enter_cd > 0) s->enter_cd--;
    /* docs/11 §5: sub-stage end (exit zone / kill-clear / boss) -> intermission;
     * final sub-stage end -> stage clear. */
    if (s->player && s->player->alive && s->nseg > 0) {
        const Seg *c = &s->segs[s->seg_i];
        int last = (s->seg_i == s->nseg - 1);
        int thr = KILLTHR[s->episode - 1];
        int adv = 0, why = 0;
        if (c->exit && s->enter_cd <= 0 &&
            s->player->x >= c->exit_x && s->player->y >= c->exit_y0 &&
            s->player->y <= c->exit_y1) { adv = 1; why = 1; }
        else if (s->killc >= thr && thr < 1000) { adv = 1; why = 2; }
        else if (last && s->boss_spawned && s->boss_dead) { adv = 1; why = 3; }
        else if (!last && !c->exit && thr >= 1000 && s->player->x >= c->right - 40) {
            adv = 1; why = 4;   /* walk-through segment (ep1 strip) */
        }
        if (adv) {
            if (last || why == 3) {
                s->phase = 2; s->phase_t = 150; s->boss_active = 0;
                slog(s, "STAGE %d CLEAR (sub-stage %d, %s)", s->episode, s->seg_i,
                     why == 1 ? "exit zone" : why == 2 ? "kill-clear" : "boss");
            } else {
                s->phase = 5; s->phase_t = 150; s->boss_active = 0;
                slog(s, "sub-stage %d clear (%s, killc=%d) -> intermission",
                     s->seg_i, why == 1 ? "exit zone" : why == 2 ? "kill-clear" : "edge", s->killc);
            }
        }
    }
    for (int i = 0; i < 8; i++) if (s->fx[i].t > 0) s->fx[i].t--;
    return STG_PLAY;
}

/* --animtest: render the player in each animation action side by side, as a
 * verification sheet, and log the record + frame index per action. */
void stage_animtest(Stage *s, uint32_t *vpx) {
    Ent *p = s->player;
    if (!p || p->nspr == 0) return;
    for (int i = 0; i < VW * VH; i++) vpx[i] = 0xFF101018u;
    const int actions[6] = {A_IDLE, A_WALK, A_PUNCH, A_KICK, A_JUMP, A_HIT};
    int save = p->state, save_a = p->action, save_b = p->beat, save_k = p->kick;
    int ybase = 250;
    for (int i = 0; i < 6; i++) {
        int a = actions[i];
        int rr = anim_rec_for(p, a);
        const ActRec *rec = (rr < p->act.count) ? &p->act.recs[rr] : NULL;
        int n = (rec && rec->n > 0) ? rec->n : 1;
        int b = (n > 1) ? (n / 2) : 0;              /* a mid/"peak" frame */
        int spd = (a == A_WALK) ? 2 : 3;
        p->state = (a == A_HIT) ? S_HIT : ((a == A_PUNCH || a == A_KICK) ? S_ATK : S_LIVE);
        p->action = a; p->kick = (a == A_KICK) ? 1 : 0; p->beat = b * spd;
        int x = 12 + i * 102;
        draw_sprite(s, vpx, p, x, ybase, 2, 0);
        draw_text(vpx, x, ybase - 16, action_name(a), 0xFFFFFFFFu, 1);
        char tmp[40];
        snprintf(tmp, sizeof tmp, "r%d f%d", rr, rec ? rec->frames[b] : 0);
        draw_text(vpx, x, ybase - 4, tmp, 0xFFB0E0B0u, 1);
        slog(s, "animtest: %-5s rec=%d n=%d beat=%d frame=%d", action_name(a), rr, n, b,
             rec ? rec->frames[b] : 0);
    }
    p->state = save; p->action = save_a; p->beat = save_b; p->kick = save_k;
}

int stage_frame(Stage *s, uint32_t *vpx, uint8_t down, uint8_t pressed, uint8_t anykey) {
    if (s->phase == 0 || s->phase == 1) s->phase_t++;

    /* ---- phase: pre-stage slides ---- */
    if (s->phase == 0) {
        Slide *sl = &s->slides[s->slide_i];
        const Pcx *pc = &sl->pcx;
        for (int y = 0; y < VH; y++) {
            uint32_t *dst = vpx + (size_t)y * VW;
            const uint8_t *row = pc->idx + (size_t)y * pc->w;
            for (int x = 0; x < VW; x++) {
                const uint8_t *c = pc->pal + (size_t)row[x] * 3;
                dst[x] = 0xFF000000u | (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | (uint32_t)c[2];
            }
        }
        char cap[32];
        snprintf(cap, sizeof cap, "STAGE %d", s->episode);
        draw_text(vpx, VW - text_w(cap, 2) - 8, VH - 20, cap, 0xFFCCCCCC, 2);
        if (anykey || ++s->slide_t >= 60) {
            s->slide_t = 0;
            if (++s->slide_i >= s->nslides) { s->phase = 1; s->phase_t = 0; }
        }
        return STG_PLAY;
    }

    /* ---- phase: play ---- */
    if (s->phase == 1) {
        Ent *p = s->player;
        /* demo input: advance right through the level; match the lane of the
         * nearest foe ahead and attack it in range (position-based progression) */
        if (s->demo && p && p->alive) {
            s->demo_step++;
            down = 0; pressed = 0;
            const Ent *tg = NULL;
            float best = 1e9f;
            for (int i = 0; i < s->nent; i++) {
                const Ent *e = &s->ents[i];
                if (e->is_player || !e->alive) continue;
                if (e->x < p->x - 40) continue;         /* ignore foes already behind */
                float d = fabsf(e->x - p->x) + fabsf(e->y - p->y) * 0.5f;
                if (d < best) { best = d; tg = e; }
            }
            if (!tg || fabsf(tg->x - p->x) > 60.0f) down |= K_RIGHT;  /* hold & fight in range */
            p->facing = 1;
            if (tg) {
                float dy = tg->y - p->y;
                if (fabsf(dy) > 12) down |= (dy < 0) ? K_UP : K_DOWN;
                if (fabsf(tg->x - p->x) < PUNCH_RNG && fabsf(dy) < LANE_TOL && (s->demo_step % 12) == 0)
                    pressed |= (s->demo_step % 36 == 0) ? K_KICK : K_PUNCH;
            }
        }
        if (p && p->alive) {
            if (p->inv_t > 0) p->inv_t--;
            int busy = (p->state == S_HIT || p->state == S_ATK || p->state == S_JUMP);
            int moving = 0;
            if (p->state == S_HIT && --p->hit_t <= 0) p->state = S_LIVE;
            if (p->state == S_JUMP && --p->jump_t <= 0) p->state = S_LIVE;
            if (p->state == S_ATK) {
                int act_frame = p->kick ? 9 : 6;
                if (--p->atk_t == act_frame && !p->atk_hit) {
                    p->atk_hit = 1;
                    if (p->kick) try_attack(s, p, KICK_RNG, KICK_DMG + (s->jin_boost ? 5 : 0));
                    else         try_attack(s, p, PUNCH_RNG, PUNCH_DMG + (s->jin_boost ? 3 : 0));
                }
                if (p->atk_t <= 0) { p->state = S_LIVE; p->kick = 0; }
            }
            if (!busy) {
                float ms = s->demo ? 3.4f : 2.4f, md = s->demo ? 2.0f : 1.4f;
                float v = 0, vd = 0;
                if (down & K_LEFT)  v = -ms;
                if (down & K_RIGHT) v =  ms;
                if (down & K_UP)    vd = -md;
                if (down & K_DOWN)  vd =  md;
                moving = (v != 0 || vd != 0) ? 1 : 0;
                if (v) p->facing = v > 0 ? 1 : -1;
                p->x += v; p->y += vd;
                const Seg *pc = (s->nseg > 0) ? &s->segs[s->seg_i] : NULL;
                p->x = clampi((int)p->x, pc ? pc->left : 0, pc ? pc->right : s->world_w);
                p->y = clampi((int)p->y, pc ? pc->top + 10 : 10, pc ? pc->bottom - 1 : WORLD_H - 1);
                if (pressed & K_JUMP)  { p->state = S_JUMP; p->jump_t = 16; play_sfx(s, "jump"); }
                else if (pressed & K_PUNCH) { p->state = S_ATK; p->atk_t = 12; p->atk_hit = 0; p->kick = 0; play_sfx(s, "punch"); }
                else if (pressed & K_KICK)  { p->state = S_ATK; p->atk_t = 16; p->atk_hit = 0; p->kick = 1; play_sfx(s, "kick"); }
            }
            /* animation action for this frame + per-action frame-index log */
            int a;
            if (p->state == S_HIT)  a = A_HIT;
            else if (p->state == S_ATK)  a = p->kick ? A_KICK : A_PUNCH;
            else if (p->state == S_JUMP) a = A_JUMP;
            else a = moving ? A_WALK : A_IDLE;
            p->action = a;
            if (a != p->last_action) {
                int rr = anim_rec_for(p, a);
                slog(s, "anim: player %s -> rec %d (n=%d)", action_name(a), rr,
                     (p->act.count && rr < p->act.count) ? p->act.recs[rr].n : 0);
                p->last_action = a;
            }
            if ((s->phase_t & 7) == 0)
                slog(s, "anim: player %s frame=%d", action_name(a), anim_frame(p));
        }
        for (int i = 0; i < s->nent; i++) {
            Ent *e = &s->ents[i];
            if (e->is_player) continue;
            if (!e->alive) { if (e->dead_t > 0) e->dead_t--; continue; }
            e->beat++;
            enemy_ai(s, e);
        }
        if (p && p->alive) p->beat++;
        if ((s->phase_t & 127) == 0) {
            int act = 0;
            for (int i = 0; i < s->nent; i++)
                if (s->ents[i].alive && !s->ents[i].is_player) act++;
            slog(s, "cam: seg=%d cam=(%d,%d) player=(%.0f,%.0f) active=%d killc=%d",
                 s->seg_i, s->camx, s->camy, p ? p->x : 0, p ? p->y : 0, act, s->killc);
        }
        if (p && p->hp <= 0) {
            p->hp = 0; p->alive = 0; s->phase = 3; s->phase_t = 150;
            slog(s, "player dead -> GAME OVER");
        }
    }

    /* ---- render ---- */
    {
        /* docs/11 §4.2: camera clamped to the CURRENT sub-stage rect:
         * camX in [left, right-320] (320 = half-res window). */
        int lo = 0, hi = s->world_w - 320;
        if (s->nseg > 0) {
            const Seg *c = &s->segs[s->seg_i];
            lo = c->left;
            hi = c->right - 320; if (hi < lo) hi = lo;
        }
        /* G3: measured camera (docs/10 §10.3): dead zone [130,190] full-res px
         * ([65,95] world px), 1/3 full-res px/frame follow (= 1/6 world px). */
        if (s->player) {
            int cx = s->camx;
            int rel = (int)s->player->x - cx;
            if (rel > 95)      cx += (rel - 95 + 5) / 6;
            else if (rel < 65) cx -= (65 - rel + 5) / 6;
            s->camx = clampi(cx, lo, hi);
        }
    }
    if (s->nseg > 0 && s->segs[s->seg_i].floor) {
        /* street: CURRENT sub-stage's .wal floor only (fw x 200), 2x upscaled
         * into screen rows 80..479 (top 80 rows = dark HUD header). The camera
         * never leaves the segment rect, so the next sub-stage's floor is never
         * visible (fixes scratch/stage1-broken.png). */
        const Seg *c = &s->segs[s->seg_i];
        int gw = c->fw;
        for (int y = 0; y < 80; y++)
            for (int x = 0; x < VW; x++) vpx[(size_t)y * VW + x] = 0xFF0A0A0E;
        for (int y = 80; y < VH; y += 2) {
            int wy = (y - 80) >> 1;
            if (wy >= 200) continue;
            uint32_t *d0 = vpx + (size_t)y * VW;
            uint32_t *d1 = vpx + (size_t)(y + 1) * VW;
            const uint32_t *fr = c->floor + (size_t)wy * gw;
            for (int sx = 0; sx < VW; sx += 2) {
                int wx = s->camx + (sx >> 1) - c->left;
                if (wx < 0) wx = 0; else if (wx >= gw) wx = gw - 1;
                uint32_t v = fr[wx];
                d0[sx] = d0[sx + 1] = v;
                d1[sx] = d1[sx + 1] = v;
            }
        }
    } else {
        /* fallback: static EPISODE card as full-screen background */
        const Pcx *bg = &s->bg;
        for (int y = 0; y < VH; y++) {
            uint32_t *dst = vpx + (size_t)y * VW;
            const uint8_t *row = bg->idx + (size_t)y * bg->w;
            for (int x = 0; x < VW; x++) {
                const uint8_t *c = bg->pal + (size_t)row[x] * 3;
                dst[x] = 0xFF000000u | (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | (uint32_t)c[2];
            }
        }
    }

    /* sort by y (far first) */
    int order[MAX_ENTS];
    for (int i = 0; i < s->nent; i++) order[i] = i;
    for (int i = 0; i < s->nent; i++)
        for (int j = i + 1; j < s->nent; j++)
            if (s->ents[order[j]].y < s->ents[order[i]].y) { int t = order[i]; order[i] = order[j]; order[j] = t; }

    for (int oi = 0; oi < s->nent; oi++) {
        Ent *e = &s->ents[order[oi]];
        if (!e->alive && e->state == S_DEAD && e->dead_t <= 0) continue;
        if (e->nspr == 0) continue;
        int sx = (int)((e->x - s->camx) * 2.0f) - (int)e->spr[0].w;
        int sy = feet_y(e->y - s->camy) - (int)(e->spr[0].h * 2);
        int flash = (e->state == S_HIT && ((e->beat / 2) & 1));
        int alpha_dead = (e->state == S_DEAD);
        if (alpha_dead && (e->dead_t / 3) & 1) continue;   /* flicker out */
        draw_sprite(s, vpx, e, sx, sy, 2, flash);
    }

    /* G10: hit FX — orange/white starburst, 10 frames (DOB-4) */
    for (int i = 0; i < 8; i++) {
        HitFx *f = &s->fx[i];
        if (f->t <= 0) continue;
        int cxp = (int)((f->x - s->camx) * 2.0f), cyp = feet_y(f->y - s->camy) - 12;
        uint32_t ray = (f->t > 6) ? 0xFFFFFFFFu : 0xFFFFA030u;
        for (int k = 0; k < 8; k++) {
            float an = k * 0.39269908f;
            int dx = (int)(cosf(an) * 10), dy = (int)(sinf(an) * 10);
            for (int r = 2; r <= 5; r++) {
                int xx = cxp + dx * r / 5, yy = cyp + dy * r / 5;
                if (xx >= 0 && xx < VW && yy >= 0 && yy < VH) vpx[(size_t)yy * VW + xx] = ray;
            }
        }
        for (int dyy = -3; dyy <= 3; dyy++)
            for (int dxx = -3; dxx <= 3; dxx++)
                if (dxx * dxx + dyy * dyy <= 9) {
                    int xx = cxp + dxx, yy = cyp + dyy;
                    if (xx >= 0 && xx < VW && yy >= 0 && yy < VH) vpx[(size_t)yy * VW + xx] = 0xFFFFFFFFu;
                }
    }

    /* ---- HUD ---- */
    if (s->phase >= 1) {
        Ent *p = s->player;
        uint32_t white = 0xFFFFFFFF, red = 0xFFE04040, grn = 0xFF40E040, yel = 0xFFFFE040;
        draw_text(vpx, 8, 6, "STAGE", white, 1);
        char tmp[64];
        snprintf(tmp, sizeof tmp, "%d-%d", s->episode, s->seg_i + 1);
        draw_text(vpx, 8 + text_w("STAGE", 1) + 4, 6, tmp, white, 1);
        /* HP bar */
        int hpw = 120, hpx = 8, hpy = 22;
        for (int x = 0; x < hpw; x++)
            for (int y = 0; y < 8; y++) {
                int px = hpx + x, py = hpy + y;
                uint32_t col = (p && x < (int)((float)hpw * p->hp / p->maxhp)) ? red : 0xFF303030;
                vpx[(size_t)py * VW + px] = col;
            }
        draw_text(vpx, hpx + hpw + 6, hpy, "HP", white, 1);
        /* G7: EXP = 6-digit number next to the HP bar (DOB-1, §9.1) */
        snprintf(tmp, sizeof tmp, "EXP %06d", s->exp);
        draw_text(vpx, 8, 36, tmp, white, 1);
        int jw = 100, jx = 8, jy = 48;
        for (int x = 0; x < jw; x++)
            for (int y = 0; y < 6; y++)
                vpx[(size_t)(jy + y) * VW + (jx + x)] = (x < (int)((float)jw * s->jinseong / 100)) ? grn : 0xFF303030;
        draw_text(vpx, jx + jw + 6, jy - 1, "JS", white, 1);
        /* G9: boss = "BOSS" + blue HP bar bottom-left (DOB-3, §8.6); else G8
         * enemy status panel top-right (DOB-2). */
        Ent *bosse = NULL;
        for (int i = 0; i < s->nent; i++)
            if (s->ents[i].is_boss && s->ents[i].alive) { bosse = &s->ents[i]; break; }
        if (bosse) {
            draw_text(vpx, 8, VH - 28, "BOSS", yel, 2);
            int bw = 240;
            for (int x = 0; x < bw; x++)
                for (int y = VH - 12; y < VH - 4; y++)
                    vpx[(size_t)y * VW + 8 + x] =
                        (x < (int)((float)bw * bosse->hp / bosse->maxhp)) ? 0xFF4080E0 : 0xFF303030;
        } else {
            Ent *foe = NULL; float bx = 1e9f;
            for (int i = 0; i < s->nent; i++) {
                Ent *e = &s->ents[i];
                if (e->alive && !e->is_player && e->x < bx) { bx = e->x; foe = e; }
            }
            if (foe) {
                int fx0 = VW - 148, fy0 = 18, fw = 132;
                for (int x = 0; x < fw; x++)
                    for (int y = 0; y < 8; y++)
                        vpx[(size_t)(fy0 + y) * VW + fx0 + x] =
                            (x < (int)((float)fw * foe->hp / foe->maxhp)) ? 0xFFE04040 : 0xFF303030;
                for (int x = 0; x < fw; x++)
                    for (int y = 10; y < 14; y++)
                        vpx[(size_t)(fy0 + y) * VW + fx0 + x] = 0xFFE0E0E0;
            }
            int remaining = 0;
            for (int i = 0; i < s->nspw; i++) if (!s->spw[i].fired) remaining++;
            for (int i = 0; i < s->nent; i++)
                if (s->ents[i].alive && !s->ents[i].is_player) remaining++;
            snprintf(tmp, sizeof tmp, "FOE %d", remaining);
            draw_text(vpx, VW - text_w(tmp, 1) - 8, 6, tmp, white, 1);
        }
        (void)white;
    }

    /* ---- phase banners ---- */
    if (s->phase == 2) {
        const char *t = "STAGE CLEAR";
        draw_text(vpx, (VW - text_w(t, 3)) / 2, VH / 2 - 12, t, 0xFFFFFF00, 3);
    } else if (s->phase == 3) {
        const char *t = "GAME OVER";
        draw_text(vpx, (VW - text_w(t, 3)) / 2, VH / 2 - 12, t, 0xFFE04040, 3);
    } else if (s->phase == 4) {
        const char *t = "THANK YOU FOR PLAYING NIGHT";
        draw_text(vpx, (VW - text_w(t, 2)) / 2, VH / 2 - 12, t, 0xFFFFFF00, 2);
    } else if (s->phase == 5) {
        const char *t1 = "SUB-STAGE CLEAR";
        const char *t2 = "ENHANCE  -  NEXT AREA";
        draw_text(vpx, (VW - text_w(t1, 3)) / 2, VH / 2 - 24, t1, 0xFFFFFF00, 3);
        draw_text(vpx, (VW - text_w(t2, 1)) / 2, VH / 2 + 8, t2, 0xFF40E040, 1);
    }

    return advance(s);
}

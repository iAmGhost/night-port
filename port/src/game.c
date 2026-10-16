/* game.c — booting vertical slice: opening.fli -> menu (scrolling
 * title1.pcx) -> stage 1 (stub, see docs/06-port.md).
 *
 * State machine mirrors FUN_0003af18 (docs/05-logic.md):
 *   opening  FUN_00038ae0  — opening.fli @ ~28 fps + CD track 1 (track02.wav)
 *   menu     FUN_0003a224  — scrolling title1.pcx + CD track 9 (track10.wav)
 *                            keys: 1=exit, 3=replay opening, else=play
 *   stage1   FUN_00037b03+ — playable stage engine (stage.c: waves, combat,
 *                            AI, HP/EXP, boss, stage progression)
 *
 * CLI:
 *   --res PATH        NIGHT.RES (default ../NIGHT/NIGHT.RES)
 *   --cdda DIR        CD-DA wav dir (default ../scratch/cdda)
 *   --state NAME      start in opening|menu|stage1 (default opening)
 *   --frames N        stop after N frames (0 = unlimited)
 *   --shot PATH       take a screenshot after --frames and exit
 *   --shot-every N    screenshot every N frames to PREFIX_f%04d.png
 *   --log PATH        append state transitions
 * Headless: SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy
 */
#include "res.h"
#include "data.h"
#include "stage.h"
#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VW 640
#define VH 480
#define FPS 30

enum { ST_OPENING, ST_MENU, ST_STAGE1, ST_QUIT };

static const char *st_name(int s) {
    switch (s) { case ST_OPENING: return "opening"; case ST_MENU: return "menu"; case ST_STAGE1: return "stage1"; }
    return "?";
}

typedef struct {
    Res  res;
    SDL_Window   *win;
    SDL_Renderer *ren;
    SDL_Texture  *tex;
    uint32_t     *vpx;              /* VW*VH ARGB8888 */

    /* opening */
    Fli    fli;
    uint8_t *fb;                    /* 320x200 */
    int      fli_done;              /* frames consumed (setup not counted) */

    /* menu */
    Pcx    title;
    int    menu_t;

    /* audio */
    SDL_AudioStream *music;
    SDL_AudioSpec   *mspec;
    SDL_AudioDeviceID audio;
    char    cdda_dir[512];

    int   state;
    int   frame;
    Stage *stage;
    int   stage_music;     /* last music track the stage requested */
    int   demo;            /* --demo: scripted stage input for headless runs */
    int   animtest;        /* --animtest: render the player animation sheet */
    int   start_episode;   /* --episode N: stage to open when --state stage1 */
    uint8_t kdown, kpress;
    int   max_frames;
    const char *shot_path;
    int   shot_every;
    char  shot_prefix[256];
    FILE  *logf;
} Game;

static void glog(Game *g, const char *fmt, ...) {
    va_list ap;
    if (!g->logf) return;
    va_start(ap, fmt);
    fprintf(g->logf, "[f%05d][%s] ", g->frame, st_name(g->state));
    vfprintf(g->logf, fmt, ap);
    fprintf(g->logf, "\n");
    va_end(ap);
    fflush(g->logf);
}

static int read_file(const char *path, uint8_t **out) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc(n > 0 ? n : 1);
    if (fread(b, 1, n, f) != (size_t)n) { fclose(f); free(b); return 0; }
    fclose(f);
    *out = b;
    return (int)n;
}

/* ----- music ----- */
static void stop_music(Game *g) {
    if (g->music) {
        SDL_UnbindAudioStream(g->music);
        SDL_DestroyAudioStream(g->music);
        g->music = NULL;
    }
}

/* CD track k (game numbering, 1-based) = file track%02d.wav of k+1 */
static int play_music(Game *g, int cd_track) {
    char path[600];
    snprintf(path, sizeof path, "%s/track%02d.wav", g->cdda_dir, cd_track + 1);
    uint8_t *d;
    int n = read_file(path, &d);
    if (n <= 0) {
        fprintf(stderr, "music: cannot read %s\n", path);
        return 0;
    }
    Wav w;
    if (!wav_open(d, (size_t)n, &w)) {
        fprintf(stderr, "music: bad wav %s\n", path);
        free(d);
        return 0;
    }
    stop_music(g);
    if (!g->mspec) {
        SDL_AudioSpec spec;
        SDL_zero(spec);
        spec.freq = w.rate;
        spec.format = SDL_AUDIO_S16LE;
        spec.channels = w.channels;
        g->audio = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (g->audio == 0) {
            fprintf(stderr, "audio: %s\n", SDL_GetError());
            wav_free(&w);
            free(d);
            return 0;
        }
        g->mspec = malloc(sizeof spec);
        *g->mspec = spec;
    }
    g->music = SDL_CreateAudioStream(g->mspec, g->mspec);
    SDL_BindAudioStream(g->audio, g->music);
    SDL_PutAudioStreamData(g->music, w.samples, (size_t)w.nframes * w.channels * 2);
    glog(g, "music: cd track %d (%s, %d frames @ %d Hz)", cd_track, path, (int)w.nframes, w.rate);
    wav_free(&w);
    free(d);
    return 1;
}

/* ----- present helpers ----- */
static void present_fli(Game *g) {
    const uint8_t *pal = g->fli.pal;
    uint32_t *o = g->vpx;
    int w = (int)g->fli.w, h = (int)g->fli.h;
    int sw = VW / w, sh = VH / h;
    for (int y = 0; y < h; y++) {
        const uint8_t *r = g->fb + (size_t)y * w;
        uint32_t *od = o + (size_t)y * sh * VW;
        for (int x = 0; x < w; x++) {
            const uint8_t *c = pal + r[x] * 3;
            /* palette stored RGB; vpx is ARGB8888 (alpha=FF) */
            uint32_t v = 0xFF000000u | (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | (uint32_t)c[2];
            for (int j = 0; j < sw; j++) od[x * sw + j] = v;
        }
        for (int j = 1; j < sh; j++) memcpy(od + (size_t)j * VW, od, (size_t)VW * 4);
    }
}

static void present_menu(Game *g) {
    /* The original menu (FUN_0003a224) settles on the full-screen title1.pcx,
     * whose bottom half bakes in "START GAME" / "EXIT TO DOS" + the 1997 credit.
     * menu_t = remaining upward offset of the image: it slides DOWN into place
     * once and STOPS at 0 (bounded, not the old infinite vertical marquee).
     * Rows outside the image fill black. */
    uint32_t *o = g->vpx;
    int mt = g->menu_t;
    for (int y = 0; y < VH; y++) {
        uint32_t *od = o + (size_t)y * VW;
        int iy = y + mt;
        if (iy < 0 || iy >= VH) { for (int x = 0; x < VW; x++) od[x] = 0; continue; }
        const uint8_t *r = g->title.idx + (size_t)iy * VW;
        for (int x = 0; x < VW; x++) {
            const uint8_t *c = g->title.pal + r[x] * 3;
            /* palette stored RGB; vpx is ARGB8888 (alpha=FF) */
            od[x] = 0xFF000000u | (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | (uint32_t)c[2];
        }
    }
    /* "START GAME" / "EXIT TO DOS": the original draws these over the title
     * (not baked into title1.pcx). Restored with the 5x7 font (docs/09). */
    draw_text(o, 310, 230, "START GAME", 0xFFFFE8C0u, 3);
    draw_text(o, 310, 274, "EXIT TO DOS", 0xFFB0A090u, 3);
}



static int take_shot(Game *g, const char *path) {
    /* Surface-based capture: works with or without a window/renderer. */
    SDL_Surface *s = SDL_CreateSurfaceFrom(VW, VH, SDL_PIXELFORMAT_ARGB8888, g->vpx, VW * 4);
    if (!s) { fprintf(stderr, "shot: %s\n", SDL_GetError()); return 0; }
    size_t npix = (size_t)s->w * s->h;
    uint8_t *rgb = malloc(npix * 3);
    const Uint8 *p = (const Uint8 *)s->pixels;
    for (size_t i = 0; i < npix; i++) {      /* ARGB8888 in memory: B G R A */
        rgb[i * 3 + 0] = p[i * 4 + 2];
        rgb[i * 3 + 1] = p[i * 4 + 1];
        rgb[i * 3 + 2] = p[i * 4 + 0];
    }
    int ok = png_write_rgb(path, s->w, s->h, rgb);
    free(rgb);
    SDL_DestroySurface(s);
    if (ok) glog(g, "shot: %s", path);
    return ok;
}

/* ----- state transitions ----- */
static void to_opening(Game *g) {
    const ResEntry *e = res_find(&g->res, "opening.fli");
    if (!e) { fprintf(stderr, "missing opening.fli\n"); exit(2); }
    fli_open(res_blob(&g->res, e), e->size, &g->fli);
    g->fb = calloc((size_t)g->fli.w * g->fli.h, 1);
    g->fli_done = 0;
    fli_next(&g->fli, g->fb);   /* setup chunk: palette + black screen */
    g->state = ST_OPENING;
    g->menu_t = 0;
    play_music(g, 1);           /* CD track 1 */
    glog(g, "enter opening (%u frames)", g->fli.frame_count);
}

static void to_menu(Game *g) {
    const ResEntry *e = res_find(&g->res, "title1.pcx");
    if (!e) { fprintf(stderr, "missing title1.pcx\n"); exit(2); }
    pcx_decode(res_blob(&g->res, e), e->size, &g->title);
    if (g->title.w != VW || g->title.h != VH)
        fprintf(stderr, "menu: title1.pcx is %dx%d, expected 640x480\n", g->title.w, g->title.h);
    g->state = ST_MENU;
    g->menu_t = 100;            /* image starts shifted up; settles down to 0 */
    play_music(g, 9);           /* CD track 9 */
    glog(g, "enter menu (keys: 1=exit 3=replay else=play)");
}

static void to_stage(Game *g, int episode) {
    if (g->stage) { stage_close(g->stage); g->stage = NULL; }
    g->state = ST_STAGE1;
    g->stage = stage_open(&g->res, episode, g->demo, g->logf);
    if (!g->stage) { fprintf(stderr, "stage_open(%d) failed\n", episode); return; }
    g->stage_music = 0;
    int intro_mt = stage_music_track(g->stage);
    play_music(g, intro_mt);
    glog(g, "enter stage%d (intro+stage music cd track %d; boss -> 12)", episode, intro_mt);
}

int game_run(int argc, char **argv) {
    Game g;
    memset(&g, 0, sizeof g);
    const char *res_path = "../NIGHT/NIGHT.RES";
    snprintf(g.cdda_dir, sizeof g.cdda_dir, "%s", "../scratch/cdda");
    g.state = ST_OPENING;
    g.max_frames = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--res") && i + 1 < argc) res_path = argv[++i];
        else if (!strcmp(argv[i], "--cdda") && i + 1 < argc) snprintf(g.cdda_dir, sizeof g.cdda_dir, "%s", argv[++i]);
        else if (!strcmp(argv[i], "--state") && i + 1 < argc) {
            if (!strcmp(argv[++i], "menu")) g.state = ST_MENU;
            else if (!strcmp(argv[i], "stage1")) g.state = ST_STAGE1;
            else g.state = ST_OPENING;
        }
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) g.max_frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--demo")) g.demo = 1;
        else if (!strcmp(argv[i], "--animtest")) g.animtest = 1;
        else if (!strcmp(argv[i], "--episode") && i + 1 < argc) g.start_episode = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--shot") && i + 1 < argc) g.shot_path = argv[++i];
        else if (!strcmp(argv[i], "--shot-every") && i + 1 < argc) {
            g.shot_every = atoi(argv[++i]);
            if (i + 2 < argc && argv[i + 1][0] != '-') snprintf(g.shot_prefix, sizeof g.shot_prefix, "%s", argv[++i]);
        }
        else if (!strcmp(argv[i], "--log") && i + 1 < argc) g.logf = fopen(argv[++i], "w");
        else { fprintf(stderr, "unknown arg %s\n", argv[i]); return 2; }
    }

    if (res_open(res_path, &g.res) != 0) { fprintf(stderr, "cannot open %s\n", res_path); return 2; }
    printf("res: %s (%u entries)\n", res_path, g.res.count);

    /* SDL 3.4.16: SDL_Init returns bool (true=success, i.e. 1). */
    if (!SDL_Init(SDL_INIT_VIDEO) || !SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    g.vpx = malloc((size_t)VW * VH * 4);
    g.win = SDL_CreateWindow("NIGHT (port)", VW, VH, SDL_WINDOW_RESIZABLE);
    if (g.win) {
        g.ren = SDL_CreateRenderer(g.win, NULL);
        if (g.ren) {
            g.tex = SDL_CreateTexture(g.ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, VW, VH);
        } else {
            fprintf(stderr, "renderer: %s (headless mode)\n", SDL_GetError());
            SDL_DestroyWindow(g.win); g.win = NULL;
        }
    } else {
        fprintf(stderr, "window: %s (headless mode)\n", SDL_GetError());
    }

    int ep = g.start_episode > 0 ? g.start_episode : 1;
    if (g.state == ST_MENU) to_menu(&g);
    else if (g.state == ST_STAGE1) to_stage(&g, ep);
    else to_opening(&g);

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = 0;
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.scancode == SDL_SCANCODE_LEFT)   g.kpress |= K_LEFT;
                else if (e.key.scancode == SDL_SCANCODE_RIGHT) g.kpress |= K_RIGHT;
                else if (e.key.scancode == SDL_SCANCODE_UP) g.kpress |= K_UP;
                else if (e.key.scancode == SDL_SCANCODE_DOWN) g.kpress |= K_DOWN;
                else if (e.key.scancode == SDL_SCANCODE_LSHIFT || e.key.scancode == SDL_SCANCODE_RSHIFT) g.kpress |= K_JUMP;
                else if (e.key.scancode == SDL_SCANCODE_LCTRL || e.key.scancode == SDL_SCANCODE_RCTRL || e.key.scancode == SDL_SCANCODE_Z) g.kpress |= K_PUNCH;
                else if (e.key.scancode == SDL_SCANCODE_LALT || e.key.scancode == SDL_SCANCODE_RALT || e.key.scancode == SDL_SCANCODE_X) g.kpress |= K_KICK;
                else g.kpress = 0;
                g.kdown |= g.kpress;
                if (g.state == ST_MENU) {
                    if (e.key.scancode == SDL_SCANCODE_1) { glog(&g, "key 1 -> exit"); running = 0; }
                    else if (e.key.scancode == SDL_SCANCODE_3) to_opening(&g);
                    else { glog(&g, "key -> play"); to_stage(&g, 1); }
                } else if (g.state == ST_OPENING) {
                    /* original FUN_0003a224 breaks the opening/title loop on any
                     * key (key flags DAT_00063ea5/DAT_00063ec0) -> skip to menu. */
                    glog(&g, "opening: key -> skip to menu");
                    to_menu(&g);
                } else if (g.state == ST_STAGE1 && e.key.scancode == SDL_SCANCODE_ESCAPE) {
                    stage_close(g.stage); g.stage = NULL; g.kdown = 0;
                    to_menu(&g);
                }
            }
            else if (e.type == SDL_EVENT_KEY_UP) {
                if (e.key.scancode == SDL_SCANCODE_LEFT)   g.kdown &= ~K_LEFT;
                else if (e.key.scancode == SDL_SCANCODE_RIGHT) g.kdown &= ~K_RIGHT;
                else if (e.key.scancode == SDL_SCANCODE_UP) g.kdown &= ~K_UP;
                else if (e.key.scancode == SDL_SCANCODE_DOWN) g.kdown &= ~K_DOWN;
                else if (e.key.scancode == SDL_SCANCODE_LSHIFT || e.key.scancode == SDL_SCANCODE_RSHIFT) g.kdown &= ~K_JUMP;
                else if (e.key.scancode == SDL_SCANCODE_LCTRL || e.key.scancode == SDL_SCANCODE_RCTRL || e.key.scancode == SDL_SCANCODE_Z) g.kdown &= ~K_PUNCH;
                else if (e.key.scancode == SDL_SCANCODE_LALT || e.key.scancode == SDL_SCANCODE_RALT || e.key.scancode == SDL_SCANCODE_X) g.kdown &= ~K_KICK;
            }
        }

        /* --animtest: render the player animation sheet once, capture, exit */
        if (g.animtest && g.state == ST_STAGE1 && g.stage) {
            stage_animtest(g.stage, g.vpx);
            take_shot(&g, g.shot_path ? g.shot_path : "scratch/shots/animtest.png");
            running = 0;
        }
        /* step */
        if (g.state == ST_OPENING) {
            if ((uint32_t)g.fli_done < g.fli.frame_count) {
                fli_next(&g.fli, g.fb);
                g.fli_done++;
            } else if ((uint32_t)g.fli_done == g.fli.frame_count) {
                g.fli_done++;
                to_menu(&g);
            }
        } else if (g.state == ST_MENU) {
            if (g.menu_t > 0) g.menu_t -= 2;   /* bounded settle: stops at 0 */
        }

        /* render */
        if (g.state == ST_OPENING) present_fli(&g);
        else if (g.state == ST_MENU) present_menu(&g);
        else if (g.stage) {
            int st = stage_frame(g.stage, g.vpx, g.kdown, g.kpress, g.kpress != 0);
            int mt = stage_music_track(g.stage);
            if (mt != g.stage_music) { g.stage_music = mt; play_music(&g, mt); glog(&g, "music -> game track %d", mt); }
            g.kpress = 0;
            if (st == STG_ADVANCE) to_stage(&g, stage_episode(g.stage) + 1);
            else if (st == STG_GAMEOVER || st == STG_VICTORY) {
                glog(&g, st == STG_GAMEOVER ? "game over -> menu" : "victory -> menu");
                stage_close(g.stage); g.stage = NULL; g.kdown = 0;
                to_menu(&g);
            }
        }
        if (g.ren) {
            SDL_UpdateTexture(g.tex, NULL, g.vpx, (size_t)VW * 4);
            SDL_SetRenderDrawColor(g.ren, 0, 0, 0, 255);
            SDL_RenderClear(g.ren);
            SDL_RenderTexture(g.ren, g.tex, NULL, NULL);
            SDL_RenderPresent(g.ren);
        }
        SDL_Delay(1000 / FPS);

        g.frame++;
        if (g.shot_every && g.frame % g.shot_every == 0) {
            char p[320];
            snprintf(p, sizeof p, "%s_f%05d_%s.png", g.shot_prefix, g.frame, st_name(g.state));
            take_shot(&g, p);
        }
        if (g.shot_path && g.frame >= g.max_frames && g.max_frames > 0) {
            take_shot(&g, g.shot_path);
            running = 0;
        } else if (g.max_frames && g.frame >= g.max_frames) {
            running = 0;
        }
    }

    glog(&g, "exit");
    if (g.logf) fclose(g.logf);
    if (g.stage) stage_close(g.stage);
    stop_music(&g);
    if (g.audio) SDL_CloseAudioDevice(g.audio);
    free(g.vpx);
    if (g.fb) free(g.fb);
    fli_free(&g.fli);
    pcx_free(&g.title);
    if (g.tex) SDL_DestroyTexture(g.tex);
    if (g.ren) SDL_DestroyRenderer(g.ren);
    if (g.win) SDL_DestroyWindow(g.win);
    res_close(&g.res);
    SDL_Quit();
    return 0;
}

/* main.c -- the SDL host for the NIGHT port.
 *
 *   make -C port app && ./port/test/night                    (windowed)
 *   SDL_VIDEODRIVER=dummy ./port/test/night --frames 600 --dump /tmp/night   (headless proof)
 *
 * ARCHITECTURE: the port runs single-threaded -- fn_03af18 (the game main) owns the call stack, and
 * the two things the original got from hardware are delivered cooperatively at the points where the
 * original waited:
 *
 *   tick   fn_03ee88 is the game's installed timer handler (it advances DAT_63e24/28/30/38/3c/40 and
 *          the PIC EOI). f.c's busy-waits are written `while (cond) { if (!sys_port_spin()) break; }`,
 *          so installing a tick pump here makes those waits real: the pump turns elapsed real time into
 *          handler calls at the rate the game programmed the PIT for (1193182 / DAT_63318, which the
 *          master init sets to 0x982 = 490 Hz; the DOS 18.2 Hz rate before that).
 *   input  the key state lives in the DS bytes the keyboard IRQ / pump fn_0347e3 used to update, so the
 *          input pump drains SDL key events into them (and calls fn_0347e3 itself).
 *
 * Presentation (g_video_ram -- the flat A0000 window the frame pumps blit the 64000-byte work screen
 * into -- plus the VGA DAC cache) runs from the tick pump at up to 60 Hz.
 */
#include "cd/cd_iso.h"
#include "f/f.h"
#include "sys/sys.h"

#include <SDL3/SDL.h>

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 320
#define SCREEN_H 200

static uint8_t   g_node_pool[0x53800];       /* the resource node pool (docs/15 boot recipe) */
static int       g_quit;
static jmp_buf   g_game_escape;              /* unwinding out of the game main for shutdown */
static int       g_escape_armed;

static SDL_Window   *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture  *g_tex;
static Uint32       *g_pixels;
static uint8_t       g_pal[768];

static int      g_frames;                    /* frames presented */
static Uint64   g_present_next;
static Uint64   g_perf_freq;

static const char *g_dump_dir;
static const int   g_dump_frames[] = { 30, 90, 240, 600, 1200, 2400, 4800, 9600 };

/* ---- fatal / terminate ---------------------------------------------- */
static void on_fatal(const char *msg) {
    SDL_Log("port fatal: %s", msg);
    g_quit = 1;
    if (g_escape_armed) longjmp(g_game_escape, 1);
}
static uint32_t on_terminate(uint32_t a) {
    SDL_Log("terminate requested (DAT_5336c/70/74 callback, arg %u)", a);
    g_quit = 1;
    if (g_escape_armed) longjmp(g_game_escape, 1);
    return 0;
}

/* ---- presentation ---------------------------------------------------- */
static void present(void) {
    if (!g_tex || !g_pixels) return;
    sys_vga_palette(g_pal);
    const uint8_t *src = g_video_ram;                       /* the A0000 window: 320x200 indexed */
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        uint8_t idx = src[i];
        g_pixels[i] = (0xffu << 24) | ((uint32_t)g_pal[idx * 3] << 16) |
                      ((uint32_t)g_pal[idx * 3 + 1] << 8) | (uint32_t)g_pal[idx * 3 + 2];
    }
    SDL_UpdateTexture(g_tex, NULL, g_pixels, SCREEN_W * 4);
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_RenderTexture(g_ren, g_tex, NULL, NULL);
    SDL_RenderPresent(g_ren);
}

static void dump_frame(int n) {
    if (!g_dump_dir) return;
    uint8_t seen[256]; memset(seen, 0, sizeof seen);
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) seen[g_video_ram[i]] = 1;
    int ndx = 0, lit = 0;
    for (int i = 0; i < 256; i++) if (seen[i]) ndx++;
    for (int i = 0; i < 768; i += 3) if (g_pal[i] | g_pal[i + 1] | g_pal[i + 2]) lit++;
    SDL_Log("frame %d: video colours=%d  dac entries lit=%d  dac writes=%u  pcx pal=%02x %02x %02x  "
            "PIT0=%u Hz tick=%u DAT_63e40=%u DAT_633a0=%u DAT_63370=%u",
            n, ndx, lit, sys_vga_palette_writes(), g_dat_636b8[0], g_dat_636b8[1], g_dat_636b8[2],
            sys_pit_hz(0), g_dat_63e24, g_dat_63e40, g_dat_633a0, g_dat_63370);
    char path[512];
    snprintf(path, sizeof path, "%s/frame%05d.bmp", g_dump_dir, n);
    SDL_Surface *s = SDL_CreateSurface(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_RGBA8888);
    if (!s) return;
    uint8_t *dst = (uint8_t *)s->pixels;
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        uint8_t idx = g_video_ram[i];
        dst[i * 4 + 0] = g_pal[idx * 3];
        dst[i * 4 + 1] = g_pal[idx * 3 + 1];
        dst[i * 4 + 2] = g_pal[idx * 3 + 2];
        dst[i * 4 + 3] = 255;
    }
    SDL_SaveBMP(s, path);
    SDL_DestroySurface(s);
    SDL_Log("wrote %s", path);
}

/* ---- input: SDL events -> the game's key state ----------------------- */
static void sdl_events(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT || ev.type == SDL_EVENT_TERMINATING) { g_quit = 1; continue; }
        if (ev.type != SDL_EVENT_KEY_DOWN && ev.type != SDL_EVENT_KEY_UP) continue;
        uint8_t sc = 0;
        switch (ev.key.scancode) {              /* the DOS scan codes the game's key state consults */
        case SDL_SCANCODE_UP:      sc = 0x48; break;
        case SDL_SCANCODE_DOWN:    sc = 0x50; break;
        case SDL_SCANCODE_LEFT:    sc = 0x4b; break;
        case SDL_SCANCODE_RIGHT:   sc = 0x4d; break;
        case SDL_SCANCODE_RETURN:  sc = 0x1c; break;
        case SDL_SCANCODE_ESCAPE:  sc = 0x01; g_quit = 1; break;
        case SDL_SCANCODE_SPACE:   sc = 0x39; break;
        case SDL_SCANCODE_LCTRL:   sc = 0x1d; break;
        case SDL_SCANCODE_LSHIFT:  sc = 0x2a; break;
        default: break;
        }
        if (!sc) continue;
        int make = ev.key.down;
        switch (sc) {
        case 0x48: g_dat_63ed3 = (uint8_t)make; break;      /* up held */
        case 0x50: g_dat_63ed5 = (uint8_t)make; break;      /* down held */
        default: break;
        }
        if (make) {
            g_dat_63eb2 = 1; g_dat_63ebe = 1;              /* the "any key" bytes */
            g_dat_62c58[sc] = 1;                           /* scan-code slot, as fn_0343d1 arms it */
        } else {
            g_dat_62c58[sc] = 0;
        }
    }
}

static void input_pump(void) {
    sdl_events();
    fn_0347e3();                                           /* the game's keyboard pump */
}

/* ---- the tick pump: real time -> the installed timer handler ---------- */
static Uint64 g_tick_next;
static void tick_pump(void) {
    Uint64 now = SDL_GetPerformanceCounter();
    if (now < g_tick_next) return;
    uint32_t hz = sys_pit_hz(0);                      /* fn_03ef29 programs PIT ch0 (0x982 -> 490 Hz) */
    if (hz == 0 && g_dat_63318) hz = 1193182u / g_dat_63318;   /* fn_03575d's ch2 user */
    if (hz == 0) hz = sys_clock_ticks_per_sec();
    if (hz == 0) hz = 18;
    Uint64 period = g_perf_freq / hz;
    uint32_t calls = 1;
    if (now - g_tick_next > period * 8) {                  /* fell far behind: re-anchor, don't burst */
        g_tick_next = now;
    } else {
        while (now >= g_tick_next && calls < 64) { g_tick_next += period; calls++; }
        calls--;
    }
    while (calls--) fn_03ee88();

    if (now >= g_present_next) {
        present();
        g_frames++;
        for (unsigned k = 0; k < sizeof g_dump_frames / sizeof g_dump_frames[0]; k++)
            if (g_frames == g_dump_frames[k]) dump_frame(g_frames);
        g_present_next = now + g_perf_freq / 60u;
    }
    if (g_quit) longjmp(g_game_escape, 1);
}

int main(int argc, char **argv) {
    const char *res_dir = "../NIGHT";
    int frames_limit = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--res-dir") && i + 1 < argc) res_dir = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) g_dump_dir = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            printf("usage: night [--res-dir DIR] [--frames N] [--dump DIR]\n");
            return 0;
        }
    }
    if (getenv("NIGHT_RES_DIR") && *getenv("NIGHT_RES_DIR")) res_dir = getenv("NIGHT_RES_DIR");
    char cue[1024];
    snprintf(cue, sizeof cue, "%s/TG_NIGHT.CUE", res_dir);

    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    g_win = SDL_CreateWindow("NIGHT", SCREEN_W * 2, SCREEN_H * 2, 0);
    g_ren = g_win ? SDL_CreateRenderer(g_win, "gpu,metal,software") : NULL;
    g_tex = g_ren ? SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                      SCREEN_W, SCREEN_H) : NULL;
    g_pixels = SDL_malloc((size_t)SCREEN_W * SCREEN_H * 4u);
    g_perf_freq = SDL_GetPerformanceFrequency();

    /* ---- port boot (the recipe the real-data test uses, docs/15) ---- */
    sys_set_fatal_handler(on_fatal);
    g_dat_5336c = g_dat_53370 = g_dat_53374 = on_terminate;
    g_dpmi_free_dos_paragraphs = 0x100000;            /* the startup free-memory gate (fn_03ae8e) */
    g_dat_52ff8 = 1;                                  /* keyboard pump enabled (fn_0347e3) */
    memset(g_node_pool, 0, sizeof g_node_pool);
    g_ndpool = g_node_pool; g_node_list_active = 0; g_node_list_done = 0;
    heap_arena_init();
    heap_arena_cap_set(48u << 20);
    heap_region_floor_set(0x10000);
    (void)heap_malloc(0x4000);                        /* one 64 KiB region floor, as the acquirer did */
    for (int t = 0; t < 0x104; t++) { g_dat_639ec[t] = 0; g_dat_53818[t] = 0; }   /* fresh-boot tables */
    sys_file_real_dir(res_dir);
    if (cd_iso_open(cue) == 0) {
        sys_cd_set_backend(cd_iso_root_header, cd_iso_root_records);
        SDL_Log("disc: %u tracks, %u sectors, lead-out MSF-derived", cd_iso_track_count(), cd_iso_total_sectors());
    } else {
        SDL_Log("disc: %s not readable -- the disc-present gate will ask for the CD", cue);
    }
    sys_set_input_pump(input_pump);
    sys_set_tick_pump(tick_pump);

    g_tick_next = SDL_GetPerformanceCounter();
    g_present_next = g_tick_next;

    /* ---- run ---- */
    if (setjmp(g_game_escape) == 0) {
        g_escape_armed = 1;
        fn_03af18();                                  /* THE GAME MAIN (0x03af18) */
        SDL_Log("the game main returned");
    } else {
        SDL_Log("game main unwound (quit / fatal)");
    }
    g_escape_armed = 0;
    g_quit = 1;
    present();

    if (frames_limit) {                               /* bounded runs: keep pumping until N frames */
        while (g_frames < frames_limit) { tick_pump(); if (g_quit && g_frames >= 2) break; }
    }
    dump_frame(g_frames);
    SDL_Log("frames presented: %d", g_frames);

    if (g_tex) SDL_DestroyTexture(g_tex);
    if (g_ren) SDL_DestroyRenderer(g_ren);
    if (g_win) SDL_DestroyWindow(g_win);
    SDL_free(g_pixels);
    cd_iso_close();
    SDL_Quit();
    return 0;
}

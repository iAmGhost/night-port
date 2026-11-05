/* main_of.c -- the openfpgaOS host for the NIGHT Pocket port.
 *
 * THE IDEA: src/f (the function-to-function port), src/sys (the DOS-service seam) and src/app/keymap.c
 * are compiled UNCHANGED from the repo; only the host is different.  src/app/main.c is the SDL3 host;
 * this file is its twin over the openfpgaOS `of_*` HAL.  Everything main.c gets from SDL it gets here
 * from the OS, and the two cooperatively-emulated services main.c installs (the PIT tick pump and the
 * keyboard pump) are installed the same way, because f.c's busy-waits are written
 *      while (cond) { if (!sys_port_spin()) break; }     (src/f/f.c:16395)
 * and only a host with a clock and an input source can make those waits real (docs/47, docs/68).
 *
 * WHAT IS HERE (task-3): presentation (320x200 room frames 1:1 into the 320x240 panel, 640x480 slide
 * frames downscaled 2:1), the palette push from the game's own DAC writes, the 490 Hz tick pump with
 * main.c's re-anchor rule, the pad->scan-code pump, the sound board's bring-up handshake, the board's
 * DMA drain into the hardware mixer, and the disc seam (of_disc.c) that answers the disc-present gate
 * from the compiled track table.
 *
 * WHAT IS NOT HERE YET: gapless CDDA (the track is advanced on the disc's own 75 sectors/s clock but
 * not sounded -- task-5, pattern = openfpgaOS-Quake src/quake/engine/cd_of.c), saves/config (task-7),
 * and the pad map's on-hardware verification (task-6).  Diagnostics that main.c prints are reduced to
 * a one-line heartbeat; the dump/shot/rig machinery is deliberately not duplicated here -- the rig and
 * the fixture suite run on the SDL host (docs/151).
 */
#include "app/keymap.h"
#include "f/f.h"
#include "sys/sys.h"

#include "of_cdda.h"
#include "of_disc.h"
#include "of_save.h"
#include "of_log.h"

#include "of.h"
#include "of_audio.h"
#include "of_cache.h"
#include "of_caps.h"
#include "of_interact_idx.h"      /* OF_IDX_* and OF_CORE_VERSION, generated with interact.json */
#include "of_file.h"
#include <dirent.h>
#include "of_input.h"
#include "of_mixer.h"
#include "of_video.h"

/* The differential rig's own witness sink (src/app/rig_sink.c) is compiled into the desktop build, so
 * a Pocket-host run produces the very frames.bin the rig's diff_screen.py / diff_state.py already read
 * (docs/151).  It is host-side diagnostics only: the device ELF never sees it. */
#ifdef OF_PC
void rig_screen_init(const char *dir, int witness_every);
void rig_screen_frame(long t_ms, unsigned long long frame, const void *pixels, int w, int h,
                      int ri, int gi, int bi);
static const char *g_rig_dir;
#endif

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- geometry (docs/330 §5) -----------------------------------------------------------
 * The game composites a 320x200 room frame (pitch 0x140, 0xfa00 bytes) and flips exactly that to the
 * framebuffer start (fn_038098), or fills the whole 640x480 region for slides (fn_03f462, count 0x1e0).
 * The panel is 320x240, so the room frame goes 1:1 with 20 rows of bar above and below, and a slide
 * frame is halved. */
#define GAME_W   320
#define GAME_H   200
#define PANEL_W  OF_SCREEN_W          /* 320 */
#define PANEL_H  OF_SCREEN_H          /* 240 */
#define BAR_ROWS ((PANEL_H - GAME_H) / 2)
/* The framebuffer mode the game picture is drawn into when it is not a 640x480 slide.  320x240 with the host
 * doing the vertical stretch (the default), or 320x200 with the core's scaler doing it (night.ini FIT=2). */
static of_video_mode_t g_fb_game;
static of_video_mode_t g_fb_native_200;
static int g_fb_200_ok;
static int g_fb_game_is_200;
/* The fit is read from the setting every frame instead of cached in a host variable.  A cached copy was a bug:
 * the desktop build skips the hardware video probe that used to set it, so night.ini FIT= / NIGHT_FIT= changed
 * nothing there and a fit comparison looked like "both runs identical".  Read live, both builds honour it. */
static int fit_wants_stretch(void) { return of_setting_fit() != 0; }

#define PRESENT_HZ   60u
#define PRESENT_PERIOD_US (1000000u / PRESENT_HZ)

/* Every host print is also written to the card's log slot (docs/335 §4): the Pocket has no console, and a
 * run that hangs or dies leaves its whole trail in Assets/night/common/night.log. */
/* Two channels, and the loud one is a fallback.  When the card log is open the trail belongs in the file and
 * the console gets only the fixed panel below -- because printing six new lines a second into a 40x30 console
 * is what produced the unreadable half-overwritten screen in the screenshots.  When the log is not openable the
 * console takes everything, as before. */
static int g_console_live = 1;
#define NLOG(...)        do { of_log_line(__VA_ARGS__); if (g_console_live) printf(__VA_ARGS__); } while (0)

/* What the OS says it is scanning out, cached rather than asked for on every frame: only this host changes the
 * mode, so refreshing after each request is enough -- and 0.1.29 proved the cache has to be honest.  With the
 * applied mode still 640x480 and the host writing 320-wide rows 640 bytes apart, source row 2k lands on the left
 * of display row k and source row 2k+1 on its right: the picture is squeezed 2:1 into the top half and stale
 * slide bytes fill the rest.  Measured on the player's screenshot (docs/343 §5): 20 all-black bar rows, and row
 * 40's left half within 7 of row 41's right half. */
static uint32_t g_applied_w = PANEL_W, g_applied_h = PANEL_H, g_applied_stride = PANEL_W;
static void applied_refresh(void) {
#ifndef OF_PC
    of_video_mode_t cur;
    of_video_get_mode(&cur);
    if (cur.width && cur.height) {
        g_applied_w = cur.width; g_applied_h = cur.height;
        g_applied_stride = cur.stride ? cur.stride : cur.width;
    }
#endif
}
static int surface_is_base(void) {
    return g_applied_stride == PANEL_W && g_applied_w == PANEL_W && g_applied_h == PANEL_H;
}
static int surface_is_200(void) {
    return g_applied_stride == GAME_W && g_applied_w == GAME_W && g_applied_h == GAME_H;
}
/* A geometry complaint once a second, never 2 280 times: a card log that repeats one line buries the run. */
static void surface_complain(const char *what, uint32_t want_rows) {
    static uint64_t last;
    uint64_t now = sys_now_us();
    if (last && now - last < 1000000ull) return;
    last = now;
    NLOG("surface: %s needs %u rows of %u-wide stride, the core has %ux%u stride=%u -- drawing into what"
         " the core gave us instead of assuming\n", what, want_rows, PANEL_W,
         g_applied_w, g_applied_h, g_applied_stride);
}

#define NLOG_STEP(what)  do { printf("phase: %s\n", (what)); of_log_line("phase: %s\n", (what)); } while (0)

static uint8_t  g_node_pool[0x53800];     /* the resource node pool (docs/15 boot recipe) */
static uint8_t  g_pal_rgb[768];
static uint32_t g_pal32[256];

static int      g_quit;
static jmp_buf  g_game_escape;
static int      g_escape_armed;
static uint32_t g_frames;

static int      g_fixed_step_calls;      /* NIGHT_RIG_STEP_CALLS=<hz>: ticks follow the frame count (docs/107) */
static long     g_fixed_fps = 60;
static long     g_frames_limit;
/* NIGHT_OFF=<list> is a fault-bisect knob: turn host subsystems off one at a time (present, mixer,
 * cdda, pad, rig) without rebuilding the game layer. */
#define OFF_PRESENT 1u
#define OFF_MIXER   2u
#define OFF_CDDA    4u
#define OFF_PAD     8u
static unsigned g_off;
static void off_init(void) {
    const char *e = getenv("NIGHT_OFF");
    if (!e) return;
    if (strstr(e, "present")) g_off |= OFF_PRESENT;
    if (strstr(e, "mixer"))   g_off |= OFF_MIXER;
    if (strstr(e, "cdda"))    g_off |= OFF_CDDA;
    if (strstr(e, "pad"))     g_off |= OFF_PAD;
    printf("host off: present=%u mixer=%u cdda=%u pad=%u\n",
           (g_off & OFF_PRESENT) ? 1u : 0u, (g_off & OFF_MIXER) ? 1u : 0u,
           (g_off & OFF_CDDA) ? 1u : 0u, (g_off & OFF_PAD) ? 1u : 0u);
}
/* The Pocket's clock (docs/335).  of_time_us() is the OS's free-running microsecond counter and is
 * 32-bit -- it wraps every 4295 s -- so it is carried into the port's 64-bit clock with a wrap counter.
 * sys.c's own fallback is clock_gettime(CLOCK_MONOTONIC), which openfpgaOS's <time.h> does not declare,
 * and its last resort clock() is a libc stub there: a port whose every wait is a deadline on sys_now_us
 * hangs or paces absurdly if that clock does not advance in microseconds. */
static uint64_t  g_clock_base;
static uint32_t  g_clock_last;
static uint64_t  g_clock_diag_last;
static uint64_t  g_clock_rate_us;      /* clock microseconds measured per wall second, for the log */
#ifndef OF_PC
static uint64_t of_clock_us(void) {
    uint32_t v = of_time_us();
    if (v < g_clock_last) g_clock_base += 0x100000000ull;
    g_clock_last = v;
    return g_clock_base + v;
}
#endif

static uint64_t g_tick_next_us;
static uint64_t g_present_next_us;
static uint64_t g_diag_next_us;
static unsigned g_timer_calls, g_pump_calls;
/* Source-framebuffer mode the OS offers (of_video_get_caps: max 800x600, 1 MB per frame).  The card reported
 * 13 modes; if a 640x480 8-bit one is among them then a changed 640x480 frame costs a 307 200-byte copy
 * (9.7 ms in the boot bench) and the core's own scaler, instead of 80 ms of majority-of-four arithmetic. */
static int  g_fb_native_ok, g_fb_native_applied;
static of_video_mode_t g_fb_native, g_fb_base;
static unsigned g_fb_mode_sw;

static unsigned g_pump_reanchor;                     /* times the tick schedule had to be re-anchored after a stall */

static void on_fatal(const char *msg) {
    printf("port fatal: %s\n", msg);
    g_quit = 1;
    if (g_escape_armed) longjmp(g_game_escape, 1);
}
static uint32_t on_terminate(uint32_t a) {
    printf("terminate requested (DAT_5336c/70/74, arg %u)\n", a);
    g_quit = 1;
    if (g_escape_armed) longjmp(g_game_escape, 1);
    return 0;
}

/* ---- palette ---------------------------------------------------------------------------
 * The game programs the VGA DAC itself (outb 0x3C8 index / 0x3C9 value, sys.c mirrors it) and never
 * writes a byte above 63 (docs/77), so a 6-bit table is scaled the way a DAC scales it -- the same
 * arithmetic src/app/main.c:129-135 uses, which is the basis the port-vs-original comparisons stand on. */
static void palette_push(void) {
    sys_vga_palette(g_pal_rgb);
    if (sys_vga_dac_max() <= 63)
        for (int i = 0; i < 768; i++)
            g_pal_rgb[i] = (uint8_t)((g_pal_rgb[i] << 2) | (g_pal_rgb[i] >> 4));
    for (int i = 0; i < 256; i++)
        g_pal32[i] = ((uint32_t)g_pal_rgb[i * 3] << 16) |
                     ((uint32_t)g_pal_rgb[i * 3 + 1] << 8) |
                     (uint32_t)g_pal_rgb[i * 3 + 2];
    of_video_palette_bulk(g_pal32, 256);
}

/* 640x480 -> 320x240 for the slide/CG path.  Averaging palette INDICES is meaningless, so the rule is
 * "the most frequent of the four, ties broken by the brightest" -- it keeps thin bright strokes (the
 * 10x12 Korean glyphs, docs/219) that a plain top-left sample would drop.  Judged on hardware
 * (docs/330 risk 2); nearest-neighbour is one #define away. */
static uint8_t downscale_2x2(const uint8_t *src, int x, int y) {
    const uint8_t *p = src + (size_t)y * 2 * 640 + (size_t)x * 2;
    uint8_t a = p[0], b = p[1], c = p[640], d = p[641];
    if (a == b && a == c && a == d) return a;
    uint8_t cand[4] = { a, b, c, d };
    uint8_t best = a; int best_n = 0; int best_lum = -1;
    for (int i = 0; i < 4; i++) {
        int n = 0;
        for (int j = 0; j < 4; j++) if (cand[j] == cand[i]) n++;
        int lum = g_pal_rgb[cand[i] * 3] + g_pal_rgb[cand[i] * 3 + 1] + g_pal_rgb[cand[i] * 3 + 2];
        if (n > best_n || (n == best_n && lum > best_lum)) { best = cand[i]; best_n = n; best_lum = lum; }
    }
    return best;
}

/* The same rule, written for the device.  The version above costs 84 ms for one 640x480 frame on the 100 MHz
 * core -- measured on hardware as "stages ... copy=84458us" -- because for every one of the 76 800 output
 * pixels it calls a function, materialises a four-byte array, runs a 16-iteration counting loop over it, and
 * reads the palette twelve times.  That single stage is why the 490 Hz tick pump only ever gets one pump per
 * present(): 84 ms of copying leaves 12 ticks a second, and a slide animated by ticks looks frozen.
 *
 * Here the four candidates come out of two aligned word loads per two output pixels, the uniform 2x2 block --
 * which is what a static slide is made of almost everywhere -- returns after three comparisons, and the
 * palette is consulted only for the ties that actually need it. */
static inline uint8_t maj4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    if (a == b && a == c && a == d) return a;
    int ab = (a == b), ac = (a == c), ad = (a == d);
    int bc = (b == c), bd = (b == d), cd = (c == d);
    int na = 1 + ab + ac + ad, nb = 1 + ab + bc + bd;
    int nc = 1 + ac + bc + cd, nd = 1 + ad + bd + cd;
    const uint8_t *p = g_pal_rgb;
    uint8_t best = a; int bestn = na;
    int bestlum = p[a * 3] + p[a * 3 + 1] + p[a * 3 + 2];
    if (nb > bestn || (nb == bestn && p[b * 3] + p[b * 3 + 1] + p[b * 3 + 2] > bestlum)) {
        best = b; bestn = nb; bestlum = p[b * 3] + p[b * 3 + 1] + p[b * 3 + 2];
    }
    if (nc > bestn || (nc == bestn && p[c * 3] + p[c * 3 + 1] + p[c * 3 + 2] > bestlum)) {
        best = c; bestn = nc; bestlum = p[c * 3] + p[c * 3 + 1] + p[c * 3 + 2];
    }
    if (nd > bestn || (nd == bestn && p[d * 3] + p[d * 3 + 1] + p[d * 3 + 2] > bestlum)) best = d;
    return best;
}

static unsigned g_ds_fast, g_ds_slow;        /* which 640x480 path ran: the word loop or the general one */

static void downscale_2x2_frame(const uint8_t *src, uint8_t *fb) {
    if (((uintptr_t)src & 3u) || ((uintptr_t)fb & 3u)) {      /* misaligned: keep the general path */
        g_ds_slow++;
        for (int y = 0; y < PANEL_H; y++)
            for (int x = 0; x < PANEL_W; x++)
                fb[y * PANEL_W + x] = downscale_2x2(src, x, y);
        return;
    }
    g_ds_fast++;
    const uint32_t *s32 = (const uint32_t *)(const void *)src;
    uint32_t *d32 = (uint32_t *)(void *)fb;
    const int srow = 2 * PANEL_W / 4, drow = PANEL_W / 4;      /* words per source / panel row */
    for (int y = 0; y < PANEL_H; y++) {
        const uint32_t *r0 = s32 + (size_t)(2 * y) * srow;
        const uint32_t *r1 = r0 + srow;
        uint32_t *out = d32 + (size_t)y * drow;
        for (int g = 0; g < drow; g++) {
            const uint32_t w00 = r0[2 * g], w01 = r0[2 * g + 1];
            const uint32_t w10 = r1[2 * g], w11 = r1[2 * g + 1];
            out[g] =
                (uint32_t)maj4((uint8_t)w00, (uint8_t)(w00 >> 8), (uint8_t)w10, (uint8_t)(w10 >> 8)) |
                ((uint32_t)maj4((uint8_t)(w00 >> 16), (uint8_t)(w00 >> 24),
                                (uint8_t)(w10 >> 16), (uint8_t)(w10 >> 24)) << 8) |
                ((uint32_t)maj4((uint8_t)w01, (uint8_t)(w01 >> 8), (uint8_t)w11, (uint8_t)(w11 >> 8)) << 16) |
                ((uint32_t)maj4((uint8_t)(w01 >> 16), (uint8_t)(w01 >> 24),
                                (uint8_t)(w11 >> 16), (uint8_t)(w11 >> 24)) << 24);
        }
    }
}

/* Panel witness: the 320x240 indexed panel as the Pocket would show it, written as a PPM the
 * analysis tools can look at.  NIGHT_PANEL_SHOT=<dir> (desktop builds only). */
#ifdef OF_PC
static void panel_shot(void) {
    static char dir[512]; static int inited, seq;
    static int limit = 0, every = 0;
    if (!limit) { limit = getenv("NIGHT_PANEL_LIMIT") ? atoi(getenv("NIGHT_PANEL_LIMIT")) : 12; if (limit < 1) limit = 1; }
    if (!every) { every = getenv("NIGHT_PANEL_EVERY") ? atoi(getenv("NIGHT_PANEL_EVERY")) : 40; if (every < 1) every = 1; }
    if (!inited) {
        const char *e = getenv("NIGHT_PANEL_SHOT");
        if (e && *e) snprintf(dir, sizeof dir, "%s", e);
        inited = 1;
    }
    if (!dir[0] || seq >= limit) return;
    char path[256];
    snprintf(path, sizeof path, "%s/%04d_%dx%d.ppm", dir, seq, PANEL_W, PANEL_H);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", PANEL_W, PANEL_H);
    const uint8_t *fb = of_video_surface();
    for (int i = 0; i < PANEL_W * PANEL_H; i++) {
        uint8_t idx = fb[i];
        fputc(g_pal_rgb[idx * 3], f); fputc(g_pal_rgb[idx * 3 + 1], f); fputc(g_pal_rgb[idx * 3 + 2], f);
    }
    fclose(f);
    seq++;
}
#endif

#ifndef OF_PC
static int g_display_mode = OF_DISPLAY_FRAMEBUFFER;
static int g_display_applied = -1;      /* what the display path was actually last asked for */
#endif

/* Stage maxima for present().  The device reports pres=110331us for a call that costs 110-335us on the desktop
 * shim, so the four things it does are timed separately: asking for the display mode, pushing the palette,
 * copying/scaling the frame into the surface, and the flush/flip pair. */
static int g_log_ok;                             /* the diagnostic slot opened (file scope: the readout reports it) */
static uint32_t g_us_mode, g_us_pal, g_us_copy, g_us_flush, g_us_flip, g_us_flip_max;
static uint32_t g_lus_mode, g_lus_pal, g_lus_copy, g_lus_flush, g_lus_flip;

/* Frame-change sampling, device path only.  The 640x480 downscale is the costliest thing this host does (the
 * device charged 84 ms for one frame of it), and a slide is static between the game's own screen updates, so
 * the source is sampled at a stride of 64 bytes and a byte-identical sample skips the copy.  The skip is
 * capped at 8 presents in a row, so a change confined to unsampled pixels cannot stay hidden for long. */
static uint32_t g_smp_off[8192];
static uint8_t g_smp_val[8192];
static size_t g_smp_n, g_smp_geom;
static unsigned g_skip_streak, g_present_skips;
static void stage_max(uint32_t *mx, uint64_t d) { if (d > (uint64_t)*mx) *mx = (uint32_t)d; }

/* Readout for the 40-column console.  A logo that the game never leaves and a game that moved on but is not
 * being shown look identical on the LCD -- so the host states, once a second, what geometry it is presenting,
 * a hash of the game's own framebuffer, how much of the palette is lit, and what a present() costs.  Lines are
 * kept short on purpose: a 100-character line wraps into unreadability at 40 columns. */
static uint32_t g_sig, g_sig_prev, g_sig_changes;
static int g_view_prev = -1, g_vw_prev = -1, g_vh_prev = -1;
static uint32_t g_present_us, g_present_us_max;

static uint32_t fb_signature(int vw, int vh) {
    const uint8_t *p = g_video_ram;
    size_t n = (size_t)vw * vh;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i += 37u) h = (h ^ p[i]) * 16777619u;
    return h;
}

static int pal_lit(void) {
    int c = 0;
    for (int i = 0; i < 256; i++)
        if (g_pal_rgb[i * 3] || g_pal_rgb[i * 3 + 1] || g_pal_rgb[i * 3 + 2]) c++;
    return c;
}

static void present(void) {
    if (g_off & OFF_PRESENT) return;
    uint64_t t0 = sys_now_us(), t1, t2, t3, t4, t5;
#ifndef OF_PC
    /* Only when it differs from what is already applied.  Re-asking the display path for its mode 59 times a
     * second was the newest thing added to this function (to survive fn_03f15f, the game's own VESA mode set),
     * and an ecall that re-programs the display is exactly the kind of thing that costs 110 ms. */
    if (g_display_mode != g_display_applied) {
        of_video_set_display_mode(g_display_mode);
        g_display_applied = g_display_mode;
    }
#endif
    t1 = sys_now_us();
    int vw = g_view_320x200 ? GAME_W : sys_video_w();
    int vh = g_view_320x200 ? GAME_H : sys_video_h();
    if (vw <= 0 || vh <= 0) return;

#ifndef OF_PC
    if (g_smp_geom != (size_t)vw * vh) {            /* (re)build the sample set when the mode changes */
        size_t k = 0;
        for (size_t o = 0; o < (size_t)vw * vh && k < sizeof g_smp_off / sizeof g_smp_off[0]; o += 64u)
            g_smp_off[k++] = (uint32_t)o;
        g_smp_n = k;
        g_smp_geom = (size_t)vw * vh;
    }
#endif

    palette_push();
    t2 = sys_now_us();
    uint8_t *fb = of_video_surface();
    const uint8_t *src = g_video_ram;               /* indexed pixels, linear in the A0000 arena */

    int skip = 0;
#ifndef OF_PC
    skip = of_setting_blit_skip() ? 1 : 0;   /* BLIT_SKIP=0 on the card keeps every blit */
    if (skip) {
        for (size_t i = 0; i < g_smp_n; i++)
            if (g_smp_val[i] != src[g_smp_off[i]]) { skip = 0; break; }
        if (g_skip_streak >= 8u) skip = 0;          /* bounded staleness, never an unbounded freeze */
    }
#endif

#ifdef OF_PC
    if (g_rig_dir) {                 /* the game's own composed picture, at its own resolution */
        static uint32_t *rgba;
        if (!rgba) rgba = (uint32_t *)malloc((size_t)vw * vh * 4);
        if (rgba) {
            for (int i = 0; i < vw * vh; i++) {
                uint8_t idx = src[i];
                rgba[i] = ((uint32_t)g_pal_rgb[idx * 3] << 24) |
                          ((uint32_t)g_pal_rgb[idx * 3 + 1] << 16) |
                          ((uint32_t)g_pal_rgb[idx * 3 + 2] << 8) | 0xffu;
            }
            rig_screen_frame((long)(sys_now_us() / 1000ull), (unsigned long long)g_frames,
                             rgba, vw, vh, 3, 2, 1);
        }
    }
#endif

    int native = 0;
#ifndef OF_PC
    g_fb_mode_sw = (unsigned)of_setting_fb_mode();
    if (!g_view_320x200 && vw == 2 * PANEL_W && vh == 2 * PANEL_H && g_fb_native_ok && g_fb_mode_sw) {
        if (!g_fb_native_applied) {
            uint64_t mt = sys_now_us();
            int rc = of_video_set_mode(&g_fb_native);
            NLOG("fb mode -> native 640x480 rc=%d in %uus\n", rc, (uint32_t)(sys_now_us() - mt));
            applied_refresh();                       /* whatever the core did, the host now knows it */
            if (rc != 0) g_fb_native_ok = 0; else g_fb_native_applied = 1;
        }
        native = g_fb_native_applied;
        if (native) fb = of_video_surface();            /* the bigger surface belongs to the new mode */
    } else if (g_fb_native_applied) {
        if (!g_fb_game.width || !g_fb_game.height) {
            /* A mode with no size is refused every time, and asking again 60 times a second is how 0.1.29 filled
             * a card log with 2 280 identical failures while the picture was wrong.  Say it once, then stop. */
            static int complained;
            if (!complained) {
                complained = 1;
                NLOG("fb mode -> game refused: the game mode is empty (%ux%u); staying in the current mode\n",
                     g_fb_game.width, g_fb_game.height);
            }
        } else {
            uint64_t mt = sys_now_us();
            int rc = of_video_set_mode(&g_fb_game);
            NLOG("fb mode -> game %ux%u rc=%d in %uus\n", g_fb_game.width, g_fb_game.height,
                 rc, (uint32_t)(sys_now_us() - mt));
            applied_refresh();
            if (rc == 0) g_fb_native_applied = 0;
        }
    }
#endif

    if (!skip) {
    if (native) {
        of_video_mode_t cur; of_video_get_mode(&cur);
        uint32_t stride = cur.stride ? cur.stride : (uint32_t)vw;
        for (int y = 0; y < vh; y++)
            memcpy(fb + (size_t)y * stride, src + (size_t)y * vw, (size_t)vw);
    } else if (vw == GAME_W && vh == GAME_H) {
        /* One loop over the rows the surface actually has, each addressed by the stride the OS actually
         * reported, with the fit deciding which source row a panel row shows.  On the surface it expects this is
         * byte-for-byte the three hand-written copies it replaces (the panel witnesses prove it); on any other
         * surface it still cannot smear, because it never writes a 320-byte row at a 640-byte offset. */
        const uint32_t stride = g_applied_stride;
        const uint32_t cols = (g_applied_w && g_applied_w < GAME_W) ? g_applied_w : GAME_W;
        const uint32_t want_rows = g_fb_game_is_200 ? GAME_H : PANEL_H;
        if (g_fb_game_is_200) { if (!surface_is_200()) surface_complain("the 320x200 scaler fit", want_rows); }
        else if (!surface_is_base()) surface_complain("the 320x240 panel fit", want_rows);
        const uint32_t rows = (g_applied_h && g_applied_h < want_rows) ? g_applied_h : want_rows;
        for (uint32_t y = 0; y < rows; y++) {
            uint8_t *d = fb + (size_t)y * stride;
            int sy;
            if (g_fb_game_is_200) sy = (int)y;                       /* 1:1 into a 320x200 surface */
            else if (fit_wants_stretch()) {
                /* 200 rows onto 240 with no bars: 40 rows shown twice, one every fifth -- the evenest row
                 * doubling there is (200 + 40 = 240 exactly), and 320x200 on a 4:3 CRT was itself shown
                 * stretched 1.2, so filling the panel is the closer picture to the original.  Five source rows
                 * fill six panel rows: the fifth is the duplicated one. */
                uint32_t grp = y / 6, r = y % 6;
                sy = (int)(5 * grp + (r < 5 ? r : 4));
            } else
                sy = ((int)y >= BAR_ROWS && (int)y < BAR_ROWS + GAME_H) ? (int)y - BAR_ROWS : -1;
            if (sy >= 0) {
                memcpy(d, src + (size_t)sy * GAME_W, cols);
                if (stride > cols) memset(d + cols, 0, stride - cols);
            } else memset(d, 0, stride);
        }
    } else if (vw == 2 * PANEL_W && vh == 2 * PANEL_H) {
        if (surface_is_base()) {
            downscale_2x2_frame(src, fb);      /* the word-at-a-time loop is written for the base surface */
        } else {
            surface_complain("the 640x480 downscale", PANEL_H);
            const uint32_t stride = g_applied_stride;
            for (uint32_t y = 0; y < g_applied_h; y++) {
                uint8_t *d = fb + (size_t)y * stride;
                for (uint32_t x = 0; x < stride; x++)
                    d[x] = (x < PANEL_W) ? downscale_2x2(src, (int)x, (int)y) : 0;
            }
        }
    } else {
        /* An unexpected mode: copy the overlapping region 1:1 rather than guess a scale. */
        int w = vw < PANEL_W ? vw : PANEL_W;
        int h = vh < PANEL_H ? vh : PANEL_H;
        memset(fb, 0, (size_t)PANEL_W * PANEL_H);
        for (int y = 0; y < h; y++)
            memcpy(fb + (size_t)y * PANEL_W, src + (size_t)y * vw, (size_t)w);
    }
    }                                        /* end of "if (!skip)" */
#ifndef OF_PC
    for (size_t i = 0; i < g_smp_n; i++) g_smp_val[i] = src[g_smp_off[i]];
    if (skip) { g_present_skips++; g_skip_streak++; } else g_skip_streak = 0;
#endif
#ifdef OF_PC
    if (getenv("NIGHT_PANEL_SHOT") && (g_frames % (unsigned)(getenv("NIGHT_PANEL_EVERY") ? atoi(getenv("NIGHT_PANEL_EVERY")) : 40)) == 20u)
        panel_shot();
#endif
    t3 = sys_now_us();
    of_video_flush();                               /* the display path reads memory the CPU just wrote */
    t4 = sys_now_us();
    of_video_flip();
#ifndef OF_PC
    /* The API is triple-buffered (of_video.h:10), and of_video_flip() does not wait: "It does not wait for the
     * vsync swap to complete, so rendering can overlap scanout" (of_video.h:171-172).  Overlapping scanout is
     * exactly a torn/ghosted picture, so the default here waits for the frame to be presented before the next
     * present() writes again; PRESENT_SYNC=0 on the card turns it off to see whether it was the cause. */
    if (of_setting_sync_flip()) of_video_wait_flip();
#endif
    t5 = sys_now_us();
    stage_max(&g_us_mode, t1 - t0);
    stage_max(&g_us_pal, t2 - t1);
    stage_max(&g_us_copy, t3 - t2);
    stage_max(&g_us_flush, t4 - t3);
    stage_max(&g_us_flip, t5 - t4);
    stage_max(&g_us_flip_max, t5 - t0);
    g_lus_mode = (uint32_t)(t1 - t0);
    g_lus_pal = (uint32_t)(t2 - t1);
    g_lus_copy = (uint32_t)(t3 - t2);
    g_lus_flush = (uint32_t)(t4 - t3);
    g_lus_flip = (uint32_t)(t5 - t4);
}

/* ---- sound board bring-up (docs/98) ----------------------------------------------------
 * The original only enables its sample player if the device answers: fn_040f4f wants the 0xaa
 * signature, fn_040fb3's probe (0xf2) must raise the device IRQ, fn_0411d9 must read an id.  This is
 * host-side device modelling, identical to src/app/main.c's board_boot(), with no SDL in it. */
#ifndef OF_PC
static void file_idle_hook(void) { of_mixer_pump(); }
#endif

static void board_cmd_hook(uint16_t port, uint8_t cmd) {
    if (port != 0x22c) return;
    if (cmd == 0xf2) sys_isr_fire(0x0f);
    if (cmd == 0xe1) {
        static const uint8_t id[] = { 0x04, 0x05 };   /* -> DAT_63f88 = 0x405, "SB 4.5" */
        sys_dev_read_seq(0x22a, id, 2);
    }
}
static void board_boot(void) {
    static const uint8_t sig[] = { 0xaa };
    sys_dev_read_seq(0x22a, sig, 1);
    sys_dev_status_set(0x22e, 0x80);
    sys_dev_status_set(0x22c, 0x00);
    sys_isr_bind(0x41439, fn_041439);
    sys_isr_bind(0x41486, fn_041486);
    sys_isr_bind(0x414d3, fn_0414d3);
    sys_isr_bind(0x41520, fn_041520);
    sys_isr_bind(0x4156d, fn_04156d);
    sys_dev_cmd_hook(board_cmd_hook);
    g_dat_53334 = 0x220;                              /* what the original runs with at the title */
}

/* ---- effects: one mixer voice per board DMA transfer (task-5 completes the music path) -----
 * sys.c models the Sound Blaster (DSP 0x40 time constant -> rate, the 8237 transfer) and turns a
 * completed transfer into s16 frames in its output ring (sys_audio_fill).  The Pocket mixer resamples
 * in fabric, so the host hands each drained run to the mixer as its own mono voice -- the same shape
 * the board had on DOS.  Sample memory must live in the mixer's SDRAM pool, and a pool block may not
 * be rewritten while its voice is still playing, hence the ping-pong. */
#define SFX_CHUNK 16384   /* frames per block.  A sound the game armed as one DMA transfer has to reach the
                           * mixer as ONE voice: at 4096 a long sound was drained as two full blocks and both
                           * were started in the same pump iteration, so the tail voice began 2 ms into the head
                           * voice and the sample played overlapped -- half its length, which is the "menu confirm
                           * cuts early" report.  The blocks are sized so no sound the port can be asked for fits
                           * twice: the largest transfer measured on the canonical route is 5348 bytes = 2674
                           * frames (docs/166), and 16384 frames is 1.5 s at 11 kHz.  8 x 32 KB of the 8 MB
                           * mixer pool.  A drain that still fills a block is counted as a split and logged, so
                           * the card log says whether this ever happens rather than leaving it to theory. */
#define SFX_PRIORITY 100  /* above the CDDA voices' priority (of_cdda.c CD_MIXER_PRIORITY 0) so a busy music
                           * track cannot starve a gameplay sound -- the rule openfpgaOS-Doom states at
                           * i_sdlsound.c:253 and the value it uses. */
#define SFX_BLOCKS 12     /* independent SFX buffers: the game overlaps sounds, and 12 x 32 KB is a small part
                           * of the 8 MB mixer pool.  The card log is what moved this from 8: plays=5 on hardware
                           * against plays=193 on the desktop shim over a comparable span meant the slots, not the
                           * samples, were the limit.  A 10 000-frame pad-only run into stage 1 arms 124 transfers
                           * of 10 distinct sizes (3088..14674 frames), so a fight really does stack them. */
static int16_t *g_sfx_pool[SFX_BLOCKS];
static int      g_sfx_voice[SFX_BLOCKS];
static int      g_sfx_block;
static int16_t  g_sfx_scratch[SFX_CHUNK];
static uint64_t g_sfx_free_us[SFX_BLOCKS];   /* when a slot's sample has run out, by our own arithmetic */
static uint32_t g_of_sfx_plays;
static uint32_t g_of_sfx_drops;
static uint32_t g_of_sfx_steals;   /* sounds we tore in half so a later one could start at all */
static int      g_sfx_volume = 200;   /* per-voice headroom, the value openfpgaOS-Doom uses; the menu's SFX
                                       * Volume is the group volume, so the two knobs do not fight */    /* samples discarded because all eight SFX slots were still sounding */   /* (f.h already exports g_sfx_plays for the game's own element-event sounds) */

static void cdda_boot(void) {
    if (of_cdda_boot() != 0) return;
    of_cdda_set_volume(of_setting_music_vol());
#ifdef OF_PC
    of_cdda_set_dir(g_rig_dir);
#endif
}

static void audio_boot(void) {
    of_mixer_init(32, OF_MIXER_OUTPUT_RATE);
    NLOG("mixer: %d voices, output rate %d Hz (samples keep their own rate, set per voice)\n", 32, OF_MIXER_OUTPUT_RATE);
#ifndef OF_PC
    /* Keep the mixer fed while a blocking data-slot read waits on its DMA (of_file.h:96-103).  The intro and
     * stage 1 are the two places the game reads NIGHT.RES hardest, and both are where the buzz appears: the
     * hook is the only thing that runs during that wait.  It must not itself read a file, so it only pumps. */
    of_file_set_idle_hook(file_idle_hook);
#endif
    for (int i = 0; i < SFX_BLOCKS; i++) {
        g_sfx_pool[i] = (int16_t *)of_mixer_alloc_samples((size_t)SFX_CHUNK * sizeof(int16_t));
        g_sfx_voice[i] = 0;
        if (!g_sfx_pool[i]) printf("mixer: pool block %d failed\n", i);
    }
    /* The setting was only ever printed.  The SFX group volume is the mixer's own knob, and the music group
     * already has one (of_cdda.c:163), so the menu's SFX Volume control has to reach it the same way. */
    of_mixer_set_group_volume(OF_MIXER_GROUP_SFX, of_setting_sfx_vol() & 0xff);
    NLOG("mixer: sfx group volume %d, %d blocks x %d frames\n",
         of_setting_sfx_vol() & 0xff, SFX_BLOCKS, SFX_CHUNK);
}

/* Fill up to one block, stopping when the ring runs dry or the block is full. */
static uint32_t drain_audio(void) {
    uint32_t got = 0;
    while (got < SFX_CHUNK) {
        uint32_t k = sys_audio_fill(g_sfx_scratch + got, SFX_CHUNK - got);
        if (!k) break;
        got += k;
    }
    return got;
}

static void audio_pump(void) {
    if (g_off & OFF_MIXER) return;
    sys_audio_set_source(g_dat_63fb4);             /* the buffer an armed DMA transfer reads */
    uint32_t rate = sys_audio_rate();
    if (!rate) rate = 22050;
    uint32_t got;
    /* Drain what the transfer has queued right now into one block.  sys_audio_fill() drains a byte ring and
     * knows nothing about sample boundaries, so filling one 1024-frame piece at a time turned a single sound
     * into a run of separate voices with a restart every 92 ms -- audible as a sample cut short. */
    while ((got = drain_audio()) != 0) {
        if (!g_sfx_pool[0]) continue;              /* no pool: still drain, the board clock must advance */
        /* A slot is free when its sample has run out -- which is arithmetic, not a question for the mixer.
         * of_mixer_voice_active() was the question, and the card answered it badly: plays=26 against drops=952
         * in one run means eight slots all looked busy at once.  A finished voice is released by the mixer only
         * when it is stopped, so a slot that is merely "done playing" still reads as active, and after eight
         * sounds every later sound is thrown away -- which is what "SFX are almost missing" was. */
        uint64_t now = sys_now_us();
        int b = -1;
        for (int i = 0; i < SFX_BLOCKS; i++) {
            int c = (g_sfx_block + i) % SFX_BLOCKS;
            if (!g_sfx_voice[c] || !of_mixer_voice_active(g_sfx_voice[c]) || now >= g_sfx_free_us[c]) {
                if (g_sfx_voice[c]) of_mixer_stop(g_sfx_voice[c]);   /* really release the mixer's slot */
                b = c;
                break;
            }
        }
        if (b < 0) {
            /* Nothing had finished.  The old rule was "drop the new sound rather than tear an old one in half",
             * and that is backwards for the sound the player just caused: a kick hit lands on top of a footstep
             * or an explosion tail and is the one you never hear.  Take the block that will finish soonest --
             * the sound least worth keeping -- and say so in the count.  openfpgaOS-Doom steals before it gives
             * up too (i_sdlsound.c:289). */
            uint64_t soon = ~0ull;
            for (int i = 0; i < SFX_BLOCKS; i++) {
                int c = (g_sfx_block + i) % SFX_BLOCKS;
                if (g_sfx_voice[c] && g_sfx_free_us[c] < soon) { soon = g_sfx_free_us[c]; b = c; }
            }
            if (b < 0) { g_of_sfx_drops++; continue; }
            of_mixer_stop(g_sfx_voice[b]);
            g_sfx_voice[b] = 0;
            g_of_sfx_steals++;
        }
        g_sfx_block = (b + 1) % SFX_BLOCKS;
        memcpy(g_sfx_pool[b], g_sfx_scratch, (size_t)got * sizeof(int16_t));
        of_cache_flush_range(g_sfx_pool[b], (size_t)got * sizeof(int16_t));
        /* Tagged as an SFX voice rather than untagged: the mixer's steal path "prefers a same-group victim
         * before crossing groups, so a busy MUSIC track can't silence an in-flight SFX (and vice versa)"
         * (of_mixer.h:322-329), and it scans the groups from opposite ends of the 32-slot range.  Untagged was
         * the old call, which put us in "other groups" and gave the mixer licence to evict whatever it liked. */
        g_sfx_voice[b] = of_mixer_alloc_for_group(OF_MIXER_GROUP_SFX, (const uint8_t *)g_sfx_pool[b],
                                                  got, rate, SFX_PRIORITY, g_sfx_volume);
#ifndef OF_PC
        if (g_sfx_voice[b] < 0) {
            /* The mixer refuses a voice only when every slot is held by an equal-or-higher priority voice.
             * Give up our own oldest sound and ask once more -- Doom does the same (i_sdlsound.c:289).  Losing
             * a sound that already started is better than losing the one the player just caused. */
            int oldest = -1; uint64_t when = ~0ull;
            for (int i = 0; i < SFX_BLOCKS; i++)
                if (g_sfx_voice[i] && g_sfx_free_us[i] < when) { when = g_sfx_free_us[i]; oldest = i; }
            if (oldest >= 0) {
                of_mixer_stop(g_sfx_voice[oldest]);
                g_sfx_voice[oldest] = 0;
                g_of_sfx_steals++;
                g_sfx_voice[b] = of_mixer_alloc_for_group(OF_MIXER_GROUP_SFX, (const uint8_t *)g_sfx_pool[b],
                                                          got, rate, SFX_PRIORITY, g_sfx_volume);
            }
        }
#endif
        /* The sample rate is set explicitly because the hardware mixer is documented (by openfpgaOS-Doom's
         * i_sdlsound.c:295-300) to possibly ignore the rate argument in the play call and use the output rate
         * instead -- 48 000 Hz.  NIGHT's samples are 10 000-11 111 Hz, so that plays every sound 4.3x too
         * fast: chirpy, and over in a fifth of the time, which is exactly "the SFX cut off before they
         * finish".  Same caution as the CDDA voices (of_cdda.c voice_start). */
        if (g_sfx_voice[b] >= 0) of_mixer_set_rate(g_sfx_voice[b], (int)rate);
        g_sfx_free_us[b] = now + (uint64_t)got * 1000000ull / rate + 20000ull;   /* +20 ms of margin */
        /* Every play now, not the first eight, and the two things that would explain a sound the player says
         * is missing or short: voice=-1 (the mixer refused it) and a drain that filled a whole block (the sound
         * was longer than one voice, so its tail is a second voice started too early). */
        NLOG("sfx#%u ch=%d rate=%u frames=%u voice=%d%s\n",
             g_of_sfx_plays, b, rate, got, g_sfx_voice[b], got == SFX_CHUNK ? " SPLIT" : "");
        g_of_sfx_plays++;
    }
}

/* ---- CD-DA (of_cdda.c) -------------------------------------------------------------
 * The engine is its own unit (of_cdda.c): two mono rings in the mixer pool, looping MUSIC voices, refilled
 * from the per-track slots, clocked at 75 sectors/s.  The host only owns the play/stop seam the game asks
 * for and the NIGHT_OFF switch that can silence it. */
/* The game's own MUSIC MODE option (DAT_00051a10 -- the byte fn_10b1d checks before it selects a CD track,
 * docs/114, and the entry the title/pause menu toggles, docs/22) only stops the game *asking* for CD-DA.  In the
 * original that is silence, because the music exists only as the reads the game issues.  Here the two MUSIC
 * voices loop over their rings until something stops them, so turning the mode off in the game's own menu
 * changed nothing at all -- which is the report.  Mirror the flag, and remember the last request so switching
 * it back on brings the same track back instead of waiting for the next scene. */
static uint32_t g_music_mode = 1;
static uint32_t g_cdda_last_index, g_cdda_last_lba, g_cdda_last_sectors;
static int cdda_allowed(void) { return !(g_off & OFF_CDDA) && g_music_mode; }
static void cdda_play(uint32_t index, uint32_t lba, uint32_t sectors) {
    if (!cdda_allowed()) return;
    g_cdda_last_index = index; g_cdda_last_lba = lba; g_cdda_last_sectors = sectors;
    of_cdda_play(index, lba, sectors);
}
static void cdda_stop(void) { if (cdda_allowed()) of_cdda_stop(); }
static void cdda_pump(void) { if (cdda_allowed()) of_cdda_pump(); }
static void music_mode_mirror(void) {
    uint32_t on = g_dat_51a10 ? 1u : 0u;
    if (on == g_music_mode) return;
    g_music_mode = on;
    if (!on) {
        of_cdda_stop();
        NLOG("music mode: off (DAT_00051a10=0) -> CD-DA voices stopped\n");
    } else if (g_cdda_last_sectors && !(g_off & OFF_CDDA)) {
        of_cdda_play(g_cdda_last_index, g_cdda_last_lba, g_cdda_last_sectors);
        NLOG("music mode: on -> track %u lba %u restarted\n",
             g_cdda_last_index, g_cdda_last_lba);
    } else {
        NLOG("music mode: on (nothing armed yet)\n");
    }
}

/* How the 320x200 game picture meets the 320x240 panel, applied at boot and whenever the menu moves.
 * fit 0 = 1:1 with 20+20 black bars (the default); 1 = the host duplicates 40 of the 200 rows so the panel is
 * filled; 2 = ask the core's scaler for its declared 320x200 framebuffer mode and let the FPGA stretch it for
 * free.  A mode change costs 41-53 ms (docs/338), so one is only issued when the choice actually changes. */
static void fit_apply(const char *why) {
    int fit = of_setting_fit();
    if (fit == 2 && !g_fb_200_ok) {
        NLOG("fit=2 (%s): the core never offered a 320x200 mode -- using the host stretch instead\n", why);
        fit = 1;
    }
    int want200 = (fit == 2);
    /* Ask for the mode only when the choice actually changes (a mode change costs 41-53 ms, docs/338) -- but
     * assign g_fb_game EVERY time.  That missing assignment is what 0.1.29 broke the core with: at the default
     * fit nothing changed, so g_fb_game stayed zero-initialised, and the present path then asked openfpgaOS for
     * framebuffer mode 0x0 on every 320x200 frame -- 2 280 "fb mode -> game 0x0 rc=-1" lines in one card log, a
     * 320x200 picture being written into a surface the core was not scanning out, and no audio because the ring
     * refill was starved behind 78 ms presents. */
    g_fb_game = g_fb_game_is_200 ? g_fb_native_200 : g_fb_base;
    if (want200 != g_fb_game_is_200) {
        const of_video_mode_t *m = want200 ? &g_fb_native_200 : &g_fb_base;
        uint64_t mt = sys_now_us();
        int rc = of_video_set_mode(m);
        NLOG("fit=%d (%s): framebuffer %ux%u rc=%d in %uus\n",
             fit, why, m->width, m->height, rc, (uint32_t)(sys_now_us() - mt));
        applied_refresh();
        if (want200 && rc != 0) {
            NLOG("fit=2 refused -- falling back to the host stretch\n");
            want200 = 0;
        }
        g_fb_game = want200 ? g_fb_native_200 : g_fb_base;
        g_fb_game_is_200 = want200;
    }
    if (!g_fb_game.width || !g_fb_game.height)
        NLOG("fit: the game framebuffer mode is empty (%ux%u) -- the host would ask the OS for a mode it cannot"
             " serve\n", g_fb_game.width, g_fb_game.height);
    NLOG("fit=%d (%s) for the 320x200 game picture on a %dx%d panel\n", of_setting_fit(),
         g_fb_game_is_200 ? "core scaler stretches 320x200 -> panel"
                          : (fit_wants_stretch() ? "host row stretch 200 -> 240 rows, no bars"
                                           : "1:1 with 20+20 black bars"),
         PANEL_W, PANEL_H);
}

#ifndef OF_PC
/* What geometry is actually being scanned out, from the OS's own answer rather than from what the host asked
 * for.  The SDK's interactdemo spells out why this matters (src/apps/interactdemo/main.c:420-436): the core's
 * scanout is a Bresenham stepper, and "a mismatch makes it skip (fb>out) or repeat (fb<out) one source pixel per
 * line, and because the accumulator carries across lines the skip WALKS down the screen".  A framebuffer whose
 * size does not match the scaler slot is therefore not a slightly wrong picture, it is a shearing one -- which
 * is the shape of the glitch 0.1.29 produced.  This reads only the SDK API, never a raw register. */
static void geo_witness(const char *why) {
    applied_refresh();
    NLOG("geo %s: applied %ux%u stride=%u | asked %ux%u | panel %dx%d\n",
         why, g_applied_w, g_applied_h, g_applied_stride, g_fb_game.width, g_fb_game.height, PANEL_W, PANEL_H);
}
#endif

#ifndef OF_PC
/* The other channel the SDK itself uses for live platform values: hardcoded MMIO on the openfpgaOS periph page.
 * interactdemo does exactly this (src/apps/interactdemo/main.c:396-436) and names what sits there --
 * INPUT_SLOT_KEY(2) at 0x40000134, HW_FEATURES at 0x40000098, MOUSE_SPEED_PCT (a SETTING) at 0x4000006C,
 * FB_MODE_SIZE at 0x400000E4, VIDEO_SCALER_MODE at 0x400000EC.  If a menu value can reach an app on this OS at
 * all, it has to be visible here, because of_interact_get() demonstrably carries nothing.
 *
 * The scan is not a hunt for names -- it is a hunt for MOTION.  Pocket LCD is the one entry the player can move
 * and the core demonstrably acts on, so whatever word changes when they move it is a live settings word, and any
 * neighbouring word the core decodes is a place a NIGHT setting could live.  Scanning only the range the SDK's
 * own demo walks keeps us inside registers we know the bitstream decodes. */
#define OF_PERIPH_BASE  0x40000000u
#define OF_PERIPH_WORDS 96                    /* 0x40000000 .. 0x4000017C */
static uint32_t g_periph_prev[OF_PERIPH_WORDS];

/* Four of those 96 words are now identified, because 0.1.38's run watched them move while the player moved the
 * menu and switched framebuffer modes:
 *
 *   0x40000080  all three Pocket LCD values appeared here in one run -- 0x00000000 On, 0x00010000 Off,
 *               0x00020000 Terminal, in bits 16-17 exactly as interact.json declares them.  openfpgaOS publishes
 *               its menu state in its OWN periph page, where an app may read it.  This is the channel the interact
 *               ecall never carried: not the APF control word (unreadable, 0.1.37), not of_interact_get() (junk),
 *               but a named OS register off the bus.
 *   0x400000e4  the applied framebuffer mode as (height<<16)|width: 0x00f00140 = 320x240, 0x01e00280 = 640x480.
 *   0x400000e8  the applied framebuffer width: 0x140 = 320, 0x280 = 640.
 *   0x400000ec  the scaler slot in use: 0 at 320x240, 7 at 640x480 -- the SDK's VIDEO_SCALER_MODE.
 *
 * The last three are worth more than a diagnostic: they say what the core actually applied, which is the question
 * docs/343 had to answer by inference. */
#define OF_PERIPH_LCD      0x40000080u
#define OF_PERIPH_FB_SIZE  0x400000e4u
#define OF_PERIPH_FB_WIDTH 0x400000e8u
#define OF_PERIPH_SCALER   0x400000ecu
static uint32_t g_periph_lcd = 0, g_periph_fbsize = 0, g_periph_fbw = 0, g_periph_scaler = 0;
static int g_periph_watch = 1;              /* the per-second full watch, bounded so it cannot eat the log slot */
#define PERIPH_LOG_LIMIT 90000

static uint32_t periph_word(uint32_t addr) { return *(volatile uint32_t *)addr; }
static unsigned periph_lcd_field(void) { return (g_periph_lcd >> 16) & 3u; }   /* 0 On, 1 Off, 2 Terminal */

static void periph_named(void) {
    uint32_t lcd = periph_word(OF_PERIPH_LCD), sz = periph_word(OF_PERIPH_FB_SIZE);
    uint32_t w = periph_word(OF_PERIPH_FB_WIDTH), sc = periph_word(OF_PERIPH_SCALER);
    if (lcd == g_periph_lcd && sz == g_periph_fbsize && w == g_periph_fbw && sc == g_periph_scaler) return;
    uint64_t t = sys_now_us();
    NLOG("periph reg t=%u.%us: lcd=%08x(%s) applied=%ux%u scaler=%u\n",
         (unsigned)(t / 1000000ull), (unsigned)((t / 100000ull) % 10ull), (unsigned)lcd,
         periph_lcd_field() == 2u ? "Terminal" : periph_lcd_field() == 1u ? "Off" : "On",
         (unsigned)(sz & 0xffffu), (unsigned)(sz >> 16), (unsigned)sc);
    g_periph_lcd = lcd; g_periph_fbsize = sz; g_periph_fbw = w; g_periph_scaler = sc;
}

static void periph_scan(int full) {
    for (int i = 0; i < OF_PERIPH_WORDS; i++) {
        uint32_t v = *(volatile uint32_t *)(OF_PERIPH_BASE + 4u * i);
        if (full && (i % 8) == 0) {
            char acc[128]; int n = 0;
            for (int j = i; j < i + 8 && j < OF_PERIPH_WORDS; j++)
                n += snprintf(acc + n, sizeof acc - (size_t)n, " %08x",
                              (unsigned)*(volatile uint32_t *)(OF_PERIPH_BASE + 4u * j));
            NLOG("periph %04x:%s\n", 4u * i, acc);
        }
        if (g_periph_prev[i] != v) {
            /* Stamped with wall time on purpose (docs/350 §4): correlating a menu change with a register needs the
             * move pinned in time, so "I moved Video H Offset at about 40 s" becomes a readable log line. */
            uint64_t t = sys_now_us();
            NLOG("periph move @0x%08x t=%u.%us: %08x -> %08x\n", OF_PERIPH_BASE + 4u * i,
                 (unsigned)(t / 1000000ull), (unsigned)((t / 100000ull) % 10ull),
                 (unsigned)g_periph_prev[i], (unsigned)v);
        }
        g_periph_prev[i] = v;
    }
}
#endif

/* The menu is live while the core runs, so the knobs are re-read on a beat instead of once at boot.
 * 100 ms is well below what a person notices on a volume slider and far above the cost of two bridge reads. */
static uint64_t g_setting_next_us;
static void settings_beat(uint64_t now) {
    if (now < g_setting_next_us) return;
    g_setting_next_us = now + 100000ull;
    int ch = of_settings_poll();
    if (ch & OF_SET_VOL_SEEN)
        NLOG("settings: music raw=%08x vol=%d\n",
             (unsigned)of_setting_music_bridge(), of_setting_music_vol());
    if (ch & OF_SET_VOL) {
        of_cdda_set_volume(of_setting_music_vol());
        of_mixer_set_group_volume(OF_MIXER_GROUP_SFX, of_setting_sfx_vol() & 0xff);
        NLOG("settings: music_vol=%d applied from the menu\n", of_setting_music_vol());
    }
    if (ch & OF_SET_LCD)
        NLOG("settings: pocket lcd raw=%08x scr=%d\n", of_setting_lcd_raw(), of_setting_screen());
    if (ch & OF_SET_FIT) {
        NLOG("settings: fit raw=%08x -> %d\n", (unsigned)of_setting_fit_bridge(), of_setting_fit());
        fit_apply("menu");
    }
}

/* ---- pad -> the shipped keyboard state (docs/91, docs/92) -------------------------------

[Showing lines 1-926 of 933 (50.0KB limit). Use offset=927 to continue.]
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

    if (!skip) {
    if (vw == GAME_W && vh == GAME_H) {
        memset(fb, 0, (size_t)BAR_ROWS * PANEL_W);
        uint8_t *dst = fb + (size_t)BAR_ROWS * PANEL_W;
        memcpy(dst, src, (size_t)GAME_W * GAME_H);
        memset(fb + (size_t)(BAR_ROWS + GAME_H) * PANEL_W, 0,
               (size_t)(PANEL_H - BAR_ROWS - GAME_H) * PANEL_W);
    } else if (vw == 2 * PANEL_W && vh == 2 * PANEL_H) {
        downscale_2x2_frame(src, fb);
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
#define SFX_CHUNK 1024
#define SFX_BLOCKS 2
static int16_t *g_sfx_pool[SFX_BLOCKS];
static int      g_sfx_voice[SFX_BLOCKS];
static int      g_sfx_block;
static int16_t  g_sfx_scratch[SFX_CHUNK];
static uint32_t g_of_sfx_plays;   /* (f.h already exports g_sfx_plays for the game's own element-event sounds) */

static void cdda_boot(void) {
    if (of_cdda_boot() != 0) return;
    of_cdda_set_volume(of_setting_music_vol());
#ifdef OF_PC
    of_cdda_set_dir(g_rig_dir);
#endif
}

static void audio_boot(void) {
    of_mixer_init(32, OF_MIXER_OUTPUT_RATE);
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
}

static void audio_pump(void) {
    if (g_off & OFF_MIXER) return;
    sys_audio_set_source(g_dat_63fb4);             /* the buffer an armed DMA transfer reads */
    uint32_t rate = sys_audio_rate();
    if (!rate) rate = 22050;
    uint32_t got;
    while ((got = sys_audio_fill(g_sfx_scratch, SFX_CHUNK)) != 0) {
        if (!g_sfx_pool[0]) continue;              /* no pool: still drain, the board clock must advance */
        int b = g_sfx_block;
        g_sfx_block = (g_sfx_block + 1) % SFX_BLOCKS;
        if (g_sfx_voice[b] && of_mixer_voice_active(g_sfx_voice[b]))
            continue;                              /* drop rather than tear a playing sample in half */
        memcpy(g_sfx_pool[b], g_sfx_scratch, (size_t)got * sizeof(int16_t));
        of_cache_flush_range(g_sfx_pool[b], (size_t)got * sizeof(int16_t));
        g_sfx_voice[b] = of_mixer_play((const uint8_t *)g_sfx_pool[b], got, rate, 100, 220);
        g_of_sfx_plays++;
    }
}

/* ---- CD-DA (of_cdda.c) -------------------------------------------------------------
 * The engine is its own unit (of_cdda.c): two mono rings in the mixer pool, looping MUSIC voices, refilled
 * from the per-track slots, clocked at 75 sectors/s.  The host only owns the play/stop seam the game asks
 * for and the NIGHT_OFF switch that can silence it. */
static void cdda_play(uint32_t index, uint32_t lba, uint32_t sectors) {
    if (g_off & OFF_CDDA) return;
    of_cdda_play(index, lba, sectors);
}
static void cdda_stop(void) { of_cdda_stop(); }
static void cdda_pump(void) { if (!(g_off & OFF_CDDA)) of_cdda_pump(); }

/* ---- pad -> the shipped keyboard state (docs/91, docs/92) -------------------------------
 * The game reads its key state from the DAT_00063e88[] held-bytes that night_key_apply() writes, and
 * that writer already aliases Z/V -> the CTRL slot, X/B -> the ALT slot, F/G/C -> the SHIFT slot, so
 * the three attacks need no modifier at all: they land on the three slots the original used.
 * Verified against the live game in task-6. */
/* of_btn() takes a BUTTON MASK (of_input_types.h: OF_BTN_A = 1 << 4), not an index. */
typedef struct { uint32_t btn; uint8_t make; } pad_key_t;
static const pad_key_t g_pad_keys[] = {
    { OF_BTN_UP,     0x48 },   /* arrows, the slots fn_0347e3 reads */
    { OF_BTN_DOWN,   0x50 },
    { OF_BTN_LEFT,   0x4b },
    { OF_BTN_RIGHT,  0x4d },
    { OF_BTN_A,      0x1d },   /* CTRL slot  = punch */
    { OF_BTN_B,      0x38 },   /* ALT  slot  = kick  */
    { OF_BTN_X,      0x2a },   /* SHIFT slot = jump  */
    { OF_BTN_Y,      0x39 },   /* Space */
    { OF_BTN_START,  0x1c },   /* Enter  (confirm) */
    { OF_BTN_SELECT, 0x01 },   /* Esc    (pause menu, docs/93) */
    { OF_BTN_L1,     0x3b },   /* F1 */
};
static uint8_t g_pad_down[sizeof g_pad_keys / sizeof g_pad_keys[0]];
static uint64_t g_pad_next_us;
static uint64_t g_pad_polls;
static uint64_t g_boot_us;                     /* the warp-entry pulse timeline starts here */

/* ---- scripted pad (task-6) -----------------------------------------------------------
 * NIGHT_PAD_SCRIPT drives the pad from the command line so the mapping can be tested without a pad:
 * comma-separated <poll>:<poll>:<name> spans, a poll being one 8 ms pad_pump() tick, names being the pad
 * buttons (up/down/left/right/a/b/x/y/start/select/l1) or a DOS scan code (0x1d).  The same spans can be
 * applied two ways -- through the pad buttons (default) or straight into night_key_apply() with
 * NIGHT_PAD_DIRECT=1 -- which is how the mapping is proven: the two runs must leave the game in the same
 * state, because the pad path may only differ from the key path in *which slot it arms*. */
typedef struct { uint64_t from, to; uint32_t btn; uint8_t sc; } pad_span_t;
static pad_span_t g_pad_script[64];
static int g_pad_script_n, g_pad_direct;

static uint32_t pad_btn_for_sc(uint8_t sc) {
    for (unsigned i = 0; i < sizeof g_pad_keys / sizeof g_pad_keys[0]; i++)
        if (g_pad_keys[i].make == sc) return g_pad_keys[i].btn;
    return 0;                                   /* no pad button reaches this slot */
}

static int g_warp_stage_cli = 0;      /* --warp-stage N: the host warp basis (docs/52) */
static int g_load_slot = -1, g_save_slot = -1, g_save_at = -1;

static void pad_script_load(void) {
    const char *e = getenv("NIGHT_PAD_SCRIPT");
    g_pad_direct = getenv("NIGHT_PAD_DIRECT") ? 1 : 0;
    if (!e) return;
    char buf[1024];
    snprintf(buf, sizeof buf, "%s", e);
    /* strtok is one global cursor, so nesting a ',' pass with a ':' pass corrupts it: split by hand. */
    for (char *tok = buf; tok && *tok && g_pad_script_n < 64; ) {
        char *comma = strchr(tok, ',');
        if (comma) *comma = 0;
        char *c1 = strchr(tok, ':');
        char *c2 = c1 ? strchr(c1 + 1, ':') : NULL;
        char *a = tok, *b = c1 ? c1 + 1 : NULL, *n = c2 ? c2 + 1 : NULL;
        if (c1) *c1 = 0;
        if (c2) *c2 = 0;
        if (!a || !b || !n) { printf("pad script: bad span '%s'\n", tok); break; }
        uint32_t btn; uint8_t sc;
        if (!strcmp(n, "up"))     { btn = OF_BTN_UP;     sc = 0x48; }
        else if (!strcmp(n, "down"))   { btn = OF_BTN_DOWN;   sc = 0x50; }
        else if (!strcmp(n, "left"))   { btn = OF_BTN_LEFT;   sc = 0x4b; }
        else if (!strcmp(n, "right"))  { btn = OF_BTN_RIGHT;  sc = 0x4d; }
        else if (!strcmp(n, "a"))      { btn = OF_BTN_A;      sc = 0x1d; }
        else if (!strcmp(n, "b"))      { btn = OF_BTN_B;      sc = 0x38; }
        else if (!strcmp(n, "x"))      { btn = OF_BTN_X;      sc = 0x2a; }
        else if (!strcmp(n, "y"))      { btn = OF_BTN_Y;      sc = 0x39; }
        else if (!strcmp(n, "start"))  { btn = OF_BTN_START;  sc = 0x1c; }
        else if (!strcmp(n, "select")) { btn = OF_BTN_SELECT; sc = 0x01; }
        else if (!strcmp(n, "l1"))     { btn = OF_BTN_L1;     sc = 0x3b; }
        else { sc = (uint8_t)strtoul(n, NULL, 0); btn = pad_btn_for_sc(sc); }
        g_pad_script[g_pad_script_n].from = strtoull(a, NULL, 0);
        g_pad_script[g_pad_script_n].to   = strtoull(b, NULL, 0);
        g_pad_script[g_pad_script_n].btn  = btn;
        g_pad_script[g_pad_script_n].sc   = sc;
        g_pad_script_n++;
        tok = comma ? comma + 1 : NULL;
    }
    printf("pad script: %d spans, direct=%d\n", g_pad_script_n, g_pad_direct);
}

/* 1 while the script holds this pad button at the current poll. */
static int pad_script_held(uint32_t btn) {
    for (int i = 0; i < g_pad_script_n; i++)
        if (g_pad_script[i].btn == btn && g_pad_polls >= g_pad_script[i].from &&
            g_pad_polls < g_pad_script[i].to)
            return 1;
    return 0;
}

/* ---- warp basis + save slots (task-7, docs/322) ---------------------------------------
 * A --warp-stage run (and a save restore, which sets g_warp_stage from the blob) has to get *inside* the
 * stage: fn_037b03 only ever runs once a game has started, and the shipped start path is a chain of key
 * waits -- logo hold, opening .fli, title wipe, menu -- where each phase consumes one press.  So a short
 * Enter pulse is posted until the player action resolver (fn_03272f) starts owning frames, which is the
 * signal the SDL host uses for the same job (src/app/main.c's warp_entry_key).  The pulse stops there so a
 * restore never injects a key into gameplay. */
static int g_warp_entry_done, g_warp_entry_posts;

static void warp_entry_pump(void) {
    if (g_warp_entry_done || f_rig_warp_stage() <= 0) return;
    if (f_act_resolver_calls() > 0) {                       /* the stage loop is live */
        g_warp_entry_done = 1;
        g_dat_63ea4 = 0; g_dat_62c58[0x1c] = 0;
        printf("warp entry: stage loop reached after %d confirm pulses (fn_03272f calls %lu)\n",
               g_warp_entry_posts, (unsigned long)f_act_resolver_calls());
        return;
    }
    uint64_t t_us = sys_now_us() - g_boot_us;
    if (t_us > 90000000ull) {
        g_warp_entry_done = 1;
        g_dat_63ea4 = 0; g_dat_62c58[0x1c] = 0;
        printf("warp entry FAILED: no stage loop after %d pulses (stage=%u)\n",
               g_warp_entry_posts, (unsigned)f_rig_warp_stage());
        return;
    }
    int on = (t_us > 500000ull) && ((t_us % 500000ull) < 250000ull);   /* 250 ms press, 250 ms release */
    if (on && g_dat_63ea4 == 0) g_warp_entry_posts++;
    g_dat_63ea4 = (uint8_t)(on ? 1 : 0);
    g_dat_62c58[0x1c] = (uint8_t)(on ? 1 : 0);
}

static void save_pump(void) {
    if (g_load_slot >= 0 && f_act_resolver_calls() > 0) {
        if (of_save_apply()) g_load_slot = -1;
    }
    if (g_save_slot >= 0 && g_save_at >= 0 && (int)g_frames == g_save_at) {
        if (of_save_write(g_save_slot) != 0 && of_save_last_error()) printf("save: %s\n", of_save_last_error());
        g_save_slot = -1;
    }
}

/* L1 held + Select saves the current slot, L1 + Start loads it, L1 + A cycles to the next slot.
 * L1 is F1 to the game and Select/Start are Esc/Enter, so the combo also reaches the shipped key state --
 * that is unavoidable on 11 buttons and is why the game sees a pause-menu key press on a save. */
static void pad_combo(void) {
    if (!pad_script_held(OF_BTN_L1) && !of_btn(OF_BTN_L1)) return;
    static uint8_t seen_select, seen_start, seen_a;
    int s = pad_script_held(OF_BTN_SELECT) || of_btn(OF_BTN_SELECT);
    int st = pad_script_held(OF_BTN_START) || of_btn(OF_BTN_START);
    int a = pad_script_held(OF_BTN_A) || of_btn(OF_BTN_A);
    if (s && !seen_select) { seen_select = 1; if (of_save_write(of_save_slot()) != 0 && of_save_last_error()) printf("save: %s\n", of_save_last_error()); }
    if (!s) seen_select = 0;
    if (st && !seen_start) { seen_start = 1; if (of_save_read(of_save_slot()) == 0) g_load_slot = of_save_slot(); else if (of_save_last_error()) printf("load: %s\n", of_save_last_error()); }
    if (!st) seen_start = 0;
    if (a && !seen_a) { seen_a = 1; of_save_slot_next(); }
    if (!a) seen_a = 0;
}

static void pad_pump(void) {
    if (g_off & OFF_PAD) return;
    uint64_t now = sys_now_us();
    if (now < g_pad_next_us) return;              /* one poll per ~8 ms, not one per spin unit */
    g_pad_next_us = now + 8000;
    of_input_poll();
    g_pad_polls++;

    if (g_pad_direct) {                            /* the key path, same poll grid */
        for (int i = 0; i < g_pad_script_n; i++) {
            int held = (g_pad_polls >= g_pad_script[i].from && g_pad_polls < g_pad_script[i].to) ? 1 : 0;
            static uint8_t prev[64];
            if (held == prev[i]) continue;
            prev[i] = (uint8_t)held;
            night_key_apply(g_pad_script[i].sc, held);
        }
    }

    for (unsigned i = 0; i < sizeof g_pad_keys / sizeof g_pad_keys[0]; i++) {
        int held = of_btn(g_pad_keys[i].btn) ? 1 : 0;
        if (!g_pad_direct && pad_script_held(g_pad_keys[i].btn)) held = 1;
        if (held == g_pad_down[i]) continue;
        g_pad_down[i] = (uint8_t)held;
        night_key_apply(g_pad_keys[i].make, held);
    }
    pad_combo();
}

static void input_pump(void) {
    pad_pump();
    fn_0347e3();                                   /* the game's own keyboard pump */
}

/* ---- tick pump: real time -> the timer handler the game installed ----------------------
 * main.c's rule, kept exactly (src/app/main.c:1747-1760): the rate is the PIT channel-0 divisor the
 * game programmed (0x982 -> 490 Hz), a host that fell more than 8 periods behind re-anchors instead of
 * bursting, and at most 64 handler calls per spin.  Presentation rides the same pump at 60 Hz. */
/* The pump is re-entered from the game's own spin loops (fn_00035ec2 and friends call
 * sys_port_spin, which calls this function), so its stack depth is a game property, not a host one.
 * Watching the depth is what separates "the host over-flew the stack" from "the game wrote out of
 * bounds" -- the cdda path is the one that pushes libc (fopen/fread) on top of that depth. */
static int g_depth, g_depth_max;
static void tick_pump(void) {
    uint64_t now = sys_now_us();
    g_depth++;
    if (g_depth > g_depth_max) {
        g_depth_max = g_depth;
        if ((g_depth % 64) == 0) printf("pump depth=%u\n", (unsigned)g_depth);
    }

    audio_pump();
    cdda_pump();
    warp_entry_pump();
    save_pump();
#ifndef OF_PC
    of_mixer_pump();
#endif

    /* main.c's pump order, kept: the board drain and the CD track always run, the tick handler only
     * when a tick is actually due.  Skipping this line makes the pump call itself the clock -- 10.7
     * million handler invocations per second instead of 490 (measured, and visible as the mode
     * timeline flapping). */
    if (now < g_tick_next_us) { g_depth--; return; }

    uint32_t hz = sys_pit_hz(0);                      /* fn_03ef29 programs PIT ch0 (0x982 -> 490 Hz) */
    if (hz == 0 && g_dat_63318) hz = 1193182u / g_dat_63318;
    if (hz == 0) hz = sys_clock_ticks_per_sec();
    if (hz == 0) hz = 18;
    /* The menu's "Tick Pump Hz" slider (interact.json index 1) may only slow the clock below the rate the
     * game programmed -- a deliberate slow-motion/tuning knob for the 100 MHz core, never a speed-up. */
    {
        int cap = of_setting_tick_hz();
        if (cap > 0 && cap < (int)hz) hz = (uint32_t)cap;
    }
    uint64_t period = 1000000ull / hz;

    unsigned calls = 1;
    if (now - g_tick_next_us > period * 8ull) {       /* fell far behind: re-anchor, don't burst */
        g_tick_next_us = now;
        g_pump_reanchor++;                            /* a stall inside game code shows up here */
    } else {
        while (now >= g_tick_next_us && calls < 64u) { g_tick_next_us += period; calls++; }
        calls--;
    }

#ifdef OF_PC
    if (g_fixed_step_calls) {
        /* Same rule as src/app/main.c: the handler count is a function of the PRESENTED frame count. */
        static unsigned s_step_frames;
        calls = 0;
        if (g_frames != s_step_frames) {
            s_step_frames = g_frames;
            unsigned want = (unsigned)((unsigned long long)g_frames *
                                       (unsigned long long)g_fixed_step_calls / (unsigned long long)g_fixed_fps);
            while (g_timer_calls < want && g_timer_calls < s_step_frames * 64ul + 64ul) {
                fn_03ee88(); g_timer_calls++;
            }
        }
    } else
#endif
    while (calls--) { fn_03ee88(); g_timer_calls++; }
    g_pump_calls++;

    if (now >= g_present_next_us) {
        uint64_t t0 = sys_now_us();
        present();
        uint32_t d = (uint32_t)(sys_now_us() - t0);
        if (d > g_present_us_max) g_present_us_max = d;
        g_present_us = d;
        g_frames++;
        g_present_next_us = now + PRESENT_PERIOD_US;
        if (g_frames_limit && g_frames >= (uint32_t)g_frames_limit) g_quit = 1;
    }
    if (now >= g_diag_next_us) {
        g_diag_next_us = now + 1000000ull;
        static uint32_t s_last_ticks;
        uint64_t clock_now = sys_now_us();
        g_clock_rate_us = g_clock_diag_last ? clock_now - g_clock_diag_last : 0;
        g_clock_diag_last = clock_now;
        NLOG("memstat frames=%u ticks=%u(+%u/s) pumps=%u arena=%u/%u pit=%u Hz xfer=%u plays=%u cdda=%u/%u/%u clock=%llu\n",
               g_frames, g_timer_calls, g_timer_calls - s_last_ticks, g_pump_calls,
               heap_arena_used(), heap_arena_cap(), sys_pit_hz(0),
               sys_audio_transfers(), g_of_sfx_plays,
               of_cdda_pos(), of_cdda_sectors(), of_cdda_tracks(), (unsigned long long)g_clock_rate_us);
        s_last_ticks = g_timer_calls;

        /* The presentation half, in four short lines the LCD can actually render. */
        {
            int vw = g_view_320x200 ? GAME_W : sys_video_w();
            int vh = g_view_320x200 ? GAME_H : sys_video_h();
            g_sig = (vw > 0 && vh > 0) ? fb_signature(vw, vh) : 0u;
            if (g_sig != g_sig_prev) { g_sig_changes++; g_sig_prev = g_sig; }
            NLOG("view=%d %dx%d sig=%08lx chg=%u pal=%d\n",
                 g_view_320x200, vw, vh, (unsigned long)g_sig, g_sig_changes, pal_lit());
            if (vw != g_vw_prev || vh != g_vh_prev || g_view_320x200 != g_view_prev) {
                NLOG("video CHANGED: view=%d %dx%d (was %d %dx%d)\n",
                     g_view_320x200, vw, vh, g_view_prev, g_vw_prev, g_vh_prev);
                g_view_prev = g_view_320x200; g_vw_prev = vw; g_vh_prev = vh;
            }
            NLOG("perf pres=%uus max=%uus pump=%u ticks=%u skips=%u\n",
                 g_present_us, g_present_us_max, g_pump_calls, g_timer_calls, g_present_skips);
            NLOG("stages last m=%u p=%u c=%u f=%u x=%u | max all=%u copy=%u flip=%u\n",
                 g_lus_mode, g_lus_pal, g_lus_copy, g_lus_flush, g_lus_flip,
                 g_us_flip_max, g_us_copy, g_us_flip);
#ifndef OF_PC
            NLOG("pump reanchor=%u frames=%u hz=%u log=%d/%ld dm=%d\n",
                 g_pump_reanchor, g_frames, hz, g_log_ok, of_log_bytes(), g_display_mode);
            NLOG("menu lcd=%08x scr=%d blit fast=%u slow=%u skip=%u\n",
                 of_setting_lcd_raw(), of_setting_screen(), g_ds_fast, g_ds_slow, g_present_skips);
            /* Once a second, not once a frame: the mode is asserted at boot, but fn_03f15f (the game's own VESA
             * mode set) runs through the boot sequence and the display path is not obliged to remember what the
             * app asked for.  1 Hz keeps the assertion without the 59 Hz cost a per-frame one adds. */
            of_video_set_display_mode(g_display_mode);
            g_display_applied = g_display_mode;
#else
            NLOG("pump reanchor=%u frames=%u hz=%u log=%d/%ld\n",
                 g_pump_reanchor, g_frames, hz, g_log_ok, of_log_bytes());
#endif
            NLOG("audio cdda=%u/%u/%u plays=%u ring=%u drift=%d\n",
                 of_cdda_pos(), of_cdda_sectors(), of_cdda_tracks(), g_of_sfx_plays,
                 of_cdda_ring_ahead(), (int)of_cdda_drift());
#ifndef OF_PC
            /* The log is the good channel; the console panel is the visible one.  When the log cannot be
             * opened at all, put the lines on the console instead of nowhere. */
            g_console_live = g_log_ok ? 0 : 1;
#endif
        }
    }
    g_depth--;
    if (g_quit) longjmp(g_game_escape, 1);
}


#ifndef OF_PC
/* One-shot witness for the file bridge, printed where it can actually be read -- the console -- rather than
 * only into the file that is the subject of the investigation.  Six hardware runs wrote a log that never
 * reached the card, and a control run of openfpgaOS-Quake on the same card wrote quake.cfg (20 B) and s0.sav
 * (35 290 B) at the moment its core exited, with a data.json whose nonvolatile slot entries are byte-identical
 * to ours.  So the question is not "does the bridge write" but "what does our app see, and when does the
 * bridge commit".  This answers the first half. */

#ifndef OF_PC
/* The console readout as one fixed panel: home, clear, and rewrite the same rows in the same places every
 * second (the terminal takes printf with ANSI escapes, README:485-495).  Nothing scrolls, so one screenshot
 * holds the whole state instead of a torn half of several seconds. */
static void diag_panel(unsigned hz) {
    char head[40];
    snprintf(head, sizeof head, "NIGHT %s  f=%u t=%u", OF_CORE_VERSION, g_frames, g_timer_calls);
    printf("\x1b[H\x1b[2J");
    printf("%-39s\n", head);
    printf("ticks+%-4u pump %-5u reanch %-4u\n", g_timer_calls, g_pump_calls, g_pump_reanchor);
    printf("hz=%u pres=%uus max=%uus\n", hz, g_present_us, g_present_us_max);
    printf("st m=%u p=%u c=%u f=%u x=%u\n",
           g_lus_mode, g_lus_pal, g_lus_copy, g_lus_flush, g_lus_flip);
    printf("view=%d %dx%d pal=%d\n", g_view_320x200,
           g_view_320x200 ? GAME_W : sys_video_w(), g_view_320x200 ? GAME_H : sys_video_h(), pal_lit());
    printf("sig=%08lx chg=%u\n", (unsigned long)g_sig, g_sig_changes);
    printf("blit fast=%u slow=%u skip=%u\n", g_ds_fast, g_ds_slow, g_present_skips);
    printf("cdda=%u/%u/%u ring=%u dr=%d\n",
           of_cdda_pos(), of_cdda_sectors(), of_cdda_tracks(), of_cdda_ring_ahead(),
           (int)of_cdda_drift());
    printf("plays=%u xfer=%u arena=%u\n", g_of_sfx_plays, sys_audio_transfers(), heap_arena_used());
    printf("clock=%llu pit=%u\n", (unsigned long long)g_clock_rate_us, sys_pit_hz(0));
    printf("lcd=%08x scr=%d dm=%d\n", (unsigned)of_setting_lcd_raw(), of_setting_screen(), g_display_mode);
    printf("log=%d bytes=%ld\n", g_log_ok, of_log_bytes());
    fflush(stdout);
}
#endif

static void file_probe(void) {
    static const char *names[] = { "nightlog.sav", "night_0.sav", "night_6.sav", "NIGHT.RES", "track02.bin" };
    for (unsigned i = 0; i < sizeof names / sizeof names[0]; i++) {
        uint32_t sid = 0;
        int r = of_file_slot_find(names[i], &sid);
        NLOG("probe find \"%s\" -> %d id=%u\n", names[i], r, sid);
    }
    DIR *d = opendir("/");
    if (!d) { NLOG("probe opendir(/) FAILED\n"); }
    else {
        struct dirent *e;
        int n = 0;
        while ((e = readdir(d)) != NULL && n < 12) {
            NLOG("probe dir \"%s\" ino=%ld slot=%d\n", e->d_name, (long)e->d_ino, (int)e->d_ino - 1);
            n++;
        }
        closedir(d);
        NLOG("probe dir listed %d (cap 12)\n", n);
    }
    FILE *f = fopen("night_6.sav", "wb");        /* a game save slot, so the probe cannot touch the log */
    if (!f) { NLOG("probe fopen night_6.sav wb FAILED\n"); }
    else {
        static const char mark[] = "NIGHT-PROBE-0001";                 /* 17 bytes, NUL included */
        size_t w = fwrite(mark, 1, sizeof mark, f);
        fclose(f);                                                     /* the close a bridge commits on */
        NLOG("probe wrote %u then closed\n", (unsigned)w);
        f = fopen("night_6.sav", "rb");
        if (!f) { NLOG("probe reopen FAILED\n"); }
        else {
            char buf[32]; size_t g = fread(buf, 1, 16, f); fclose(f);
            buf[g < 16 ? g : 15] = 0;
            NLOG("probe readback %u \"%s\"\n", (unsigned)g, buf);
        }
    }
}
#endif

int main(int argc, char **argv) {
    /* A core that is killed (or that dies in a spin) must not lose its log: unbuffered stdout is how
     * this host is read on a desktop run and in the emulator log. */
    setvbuf(stdout, (char *)0, _IONBF, 0);
    off_init();

    for (int i = 1; i + 1 < argc; i++) {
        if (!strcmp(argv[i], "--frames") && i + 1 < argc) g_frames_limit = atol(argv[++i]);
#ifdef OF_PC
        else if (!strcmp(argv[i], "--rig-dump") && i + 1 < argc) g_rig_dir = argv[++i];
        else if (!strcmp(argv[i], "--warp-stage") && i + 1 < argc) g_warp_stage_cli = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--save") && i + 1 < argc) g_save_slot = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--save-at") && i + 1 < argc) g_save_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--load") && i + 1 < argc) g_load_slot = atoi(argv[++i]);
#endif
    }
    if (getenv("NIGHT_SEED")) g_rng_seed_override = atoi(getenv("NIGHT_SEED"));   /* docs/107 reproducible runs */
    g_log_ok = (of_log_open() == 0) ? 1 : 0;
    if (!g_log_ok) printf("log: the diagnostic slot is not openable; the terminal is the only trail\n");
    else atexit(of_log_close);          /* a fclose is what commits a nonvolatile slot (README:466) */
    NLOG_STEP("boot start");
    if (getenv("NIGHT_RIG_STEP_CALLS")) {
        int hz = atoi(getenv("NIGHT_RIG_STEP_CALLS"));
        if (hz > 0) g_fixed_step_calls = hz;
    }
    if (getenv("NIGHT_RIG_FPS")) {
        int f = atoi(getenv("NIGHT_RIG_FPS"));
        if (f > 0) g_fixed_fps = f;
    }
#ifndef OF_PC
    sys_set_clock_source(of_clock_us);            /* every wait in the port is a deadline on this */
#endif
    of_video_init();
    of_video_set_color_mode(OF_VIDEO_MODE_8BIT);
    of_audio_init();

    const struct of_capabilities *caps = of_get_caps();
    NLOG("night pocket: cpu=%u Hz sdram=%u KB heap=%u KB fb=%ux%u stride=%u voices=%u variant=%u\n",
           caps->cpu_freq_hz, caps->sdram_size / 1024, caps->heap_size / 1024,
           caps->fb_width, caps->fb_height, caps->fb_stride, caps->mixer_voices, caps->core_variant);
#ifndef OF_PC
    NLOG_STEP("clock source installed");
    NLOG("clock: openfpgaOS of_time_us installed; probe: ");
    {
        uint64_t a = sys_now_us();
        for (volatile int i = 0; i < 2000000; i++) ;
        uint64_t b = sys_now_us();
        NLOG("%llu us over a 2M-iteration spin (%s)\n", (unsigned long long)(b - a),
             (b > a) ? "advancing" : "NOT ADVANCING");
    }
#endif



#ifdef OF_PC
    if (g_rig_dir) rig_screen_init(g_rig_dir, 8);
#endif

    of_settings_load();                        /* os.ini [night] on the device, env on the desktop */
    NLOG("settings: music_vol=%d sfx_vol=%d cdda=%d pad=%d tick_hz=%d screen=%d\n",
         of_setting_music_vol(), of_setting_sfx_vol(), of_setting_cdda(), of_setting_pad(),
         of_setting_tick_hz(), of_setting_screen());
#ifndef OF_PC
    /* Pocket LCD (interact.json id 70, bits 16-17 of 0xF7000000) is the screen switch, and the host mirrors
     * it into of_video_set_display_mode() (of_video.h:43-45): OF_DISPLAY_TERMINAL shows the 40x30 console the
     * printf diagnostics go to, OF_DISPLAY_FRAMEBUFFER the game.  The mode is re-derived from that setting on
     * every present(), which is the point: a stuck-of-its-own default used to force OF_DISPLAY_FRAMEBUFFER
     * while the menu said Terminal, i.e. the port overrode the user 60 times a second (docs/336). */
    {
        int scr = of_setting_screen();
        int mode = (scr == 1) ? OF_DISPLAY_TERMINAL : (scr == 2) ? OF_DISPLAY_OVERLAY : OF_DISPLAY_FRAMEBUFFER;
        /* Not overridden any more.  This used to read "if the log slot is not openable, force OVERLAY", and on
         * the device the log slot is not openable -- so every run that was not asked for Terminal came up as
         * OF_DISPLAY_OVERLAY (of_video.h:44-46: "white terminal text over framebuffer"), which is exactly what
         * the screenshots show: text over the game while the menu says On.  The game screen is what the menu
         * asked for; a log failure is reported, it is not compensated for by changing the user's display. */
        g_display_mode = mode;
        g_display_applied = mode;
        of_video_set_display_mode(mode);
        NLOG("screen: ini/menu=%d display_mode=%d (0=terminal 1=framebuffer 2=overlay)\n", scr, mode);
        NLOG("log ok=%d bytes=%ld\n", g_log_ok, of_log_bytes());
        NLOG("blit src=%p fb=%p al=%d/%d\n", (void *)g_video_ram, (void *)of_video_surface(),
             (int)((uintptr_t)g_video_ram & 3u), (int)((uintptr_t)of_video_surface() & 3u));
        file_probe();
#ifndef OF_PC
        {   /* What the file layer thinks of the name we open the log on. */
            uint32_t sid = 0;
            int fr = of_file_slot_find("night.cfg", &sid);
            NLOG("file: slot_find(night.cfg)=%d id=%u\n", fr, sid);
        }
#endif
        NLOG("================================================\n");
        NLOG("=== NIGHT (openfpgaOS) -- console overlay test ===\n");
        NLOG("================================================\n");
    }
#endif
    if (!of_setting_cdda()) g_off |= OFF_CDDA;
    if (!of_setting_pad())  g_off |= OFF_PAD;
    if (g_load_slot >= 0) {                    /* a restore arms the warp basis from the blob */
        if (of_save_read(g_load_slot) != 0 && of_save_last_error()) printf("load: %s\n", of_save_last_error());
    }
    if (g_warp_stage_cli > 0) g_warp_stage = g_warp_stage_cli;
    pad_script_load();
    NLOG_STEP("board_boot");
    board_boot();
    NLOG_STEP("audio_boot");
    audio_boot();
    NLOG_STEP("cdda_boot");
    cdda_boot();
    sys_set_res_hook(0);

    /* ---- the boot recipe the real-data test uses (docs/15), host for host with src/app/main.c ---- */
    sys_set_fatal_handler(on_fatal);
    g_dat_5336c = g_dat_53370 = g_dat_53374 = on_terminate;
    g_dpmi_free_dos_paragraphs = 0x100000;         /* the startup free-memory gate (fn_03ae8e) */
    g_dat_52ff8 = 1;                               /* keyboard pump enabled (fn_0347e3) */
    memset(g_node_pool, 0, sizeof g_node_pool);
    g_ndpool = g_node_pool; g_node_list_active = 0; g_node_list_done = 0;
    heap_arena_init();
    heap_arena_cap_set(NIGHT_HEAP_ARENA_MAX);      /* shrunk for the Pocket build by -D (docs/330 §4) */
    heap_region_floor_set(0x10000);
    (void)heap_malloc(0x4000);
    for (int t = 0; t < 0x104; t++) { g_dat_639ec[t] = 0; g_dat_53818[t] = 0; }

    /* Resource opens (fn_04270d / fn_036853) resolve to "<dir>/<name>".  On the device the kernel
     * resolves names to data slots through the virtual root, so "/" + "NIGHT.RES" is slot 4; on the
     * desktop build it is the repo's NIGHT/ directory. */
#ifndef OF_PC
    sys_file_real_dir("/");
#else
    /* Desktop: find the data directory rather than assume the cwd.  The same mistake is what the
     * game itself reports ("NIGHT.RES File Not Found!!!" -> fn_036537 -> terminate), so the host
     * checks each candidate with the file the loader actually needs. */
    {
        static const char *cand[] = { "NIGHT", "../NIGHT", "../../NIGHT", "../../../NIGHT",
                                      "../../../../NIGHT" };
        const char *dir = getenv("NIGHT_RES_DIR");
        if (!dir || !*dir) {
            dir = cand[0];
            for (unsigned i = 0; i < sizeof cand / sizeof cand[0]; i++) {
                char probe[512];
                snprintf(probe, sizeof probe, "%s/NIGHT.RES", cand[i]);
                FILE *f = fopen(probe, "rb");
                if (f) { fclose(f); dir = cand[i]; break; }
            }
        }
        printf("res dir: %s\n", dir);
        sys_file_real_dir(dir);
    }
#endif

    int tracks = of_disc_open();
    sys_cd_set_audio_backend(cdda_play, cdda_stop);
    NLOG("disc: %d tracks, %u sectors\n", tracks, of_disc_total_sectors());

    sys_set_input_pump(input_pump);
    sys_set_tick_pump(tick_pump);
    sys_set_present_hook(present);                 /* docs/325: a pass that writes 0xA0000 and waits asks for one present */

    g_tick_next_us = sys_now_us();
    g_present_next_us = g_tick_next_us;
    g_diag_next_us = g_tick_next_us + 1000000ull;

    g_boot_us = sys_now_us();                       /* the warp-entry pulse timeline (task-7) */
    if (setjmp(g_game_escape) == 0) {
        g_escape_armed = 1;
        NLOG_STEP("game main entered");
        dispatch_a(0xff);                          /* 0x422c7: the registered event-table entries */
        fn_0445c9_startup();                       /* 0x0445d8-0445fe: the entry thunk's pre-main half */
        fn_03af18();                               /* THE GAME MAIN */
        NLOG("the game main returned\n");
    } else {
        NLOG("game main unwound (quit / fatal)\n");
    }
    g_escape_armed = 0;
    g_quit = 1;
    of_cdda_stop();                 /* closes the witness WAV (and its header fix-up) on the way out */
    present();
    printf("frames presented: %u  ticks: %u  cdda tracks: %u  sfx voices: %u\n",
           g_frames, g_timer_calls, of_cdda_tracks(), g_of_sfx_plays);
    return 0;
}

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
/* The state half of the same sink: the DS cell map, the record blocks and the entity walk, which is what
 * scripts/rig/diff_state.py and scripts/rig/ent_list.py read.  Without it a Pocket-host dump only had frames,
 * so the pocket path could not be compared with the SDL host on state -- only on pictures (docs/333 compared
 * mean RGB because that was all it had). */
void rig_init(const char *dir, int hz);
void rig_poll(long t_ms, unsigned long long frame, unsigned long long audio_bytes);
void rig_close(void);
void rig_marker(long t_ms, const char *fmt, ...);   /* rig_sink.c, no header (src/app/main.c:624 the same way) */
static const char *g_rig_dir;
#ifdef OF_PC
static void rig_res_marker(const char *name) {
    if (g_rig_dir) rig_marker((long)(sys_now_us() / 1000ull), "res %s", name ? name : "?");
}
#endif
static int g_rig_hz = 10;                      /* src/app/main.c's own default, so both hosts stamp the same grid */
static int g_rig_witness = 8;
#endif

#include <setjmp.h>
#include <sys/stat.h>   /* mkdir for the rig's arena witness (the docs/62 comparison basis) */
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
/* The budget line needs a MEAN, and `pres=` only ever carried the last frame and the worst one: a run whose
 * typical present is 3.6 ms and whose worst is 75 ms looks identical to one that is 3.6/7.5 in that line. */
static uint64_t g_present_us_sum;
static uint32_t g_present_n;

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
    /* docs/62's comparison basis, on the Pocket path: the ARENA bytes behind the picture, not the composed
     * bitmap.  Index-vs-index against the shipped PCX is byte-exact and says nothing about the palette, which
     * is the point -- fn_040660 ramps a card in over 0x19 steps, so an RGB sample mid-fade cannot match the
     * card's trailer palette at all, while the arena is identical from the first frame that holds the card.
     * Bounded on purpose: every 8th presented frame, at most 120 raws, so a nine-stage sweep is tens of MB. */
    static int g_vidraw = -1;
    static int g_raw_count;
    if (g_vidraw < 0) g_vidraw = getenv("NIGHT_VIDRAW") ? 1 : 0;
    if (g_vidraw && g_rig_dir && (g_frames % 8u) == 0u && g_raw_count < 120) {
        char sub[1200], rpath[1400];
        snprintf(sub, sizeof sub, "%s/arena", g_rig_dir);
        mkdir(sub, 0777);
        snprintf(rpath, sizeof rpath, "%s/%06llu_%dx%d.raw", sub, (unsigned long long)g_frames, vw, vh);
        FILE *rf = fopen(rpath, "wb");
        if (rf) {
            fwrite(src, 1, (size_t)vw * vh, rf);
            ptrdiff_t delta = (ptrdiff_t)((const uint8_t *)(uintptr_t)g_dat_633b0 - (const uint8_t *)g_video_ram);
            fprintf(rf, "#base_delta=%td#ents=%d#pal", delta, (int)g_dat_53c68);
            fwrite(g_pal_rgb, 1, 768, rf);
            fclose(rf);
            g_raw_count++;
        }
    }

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
/* Headroom, or the lack of it.  200 was openfpgaOS-Doom's number and Doom has a reason for it: 32 simultaneous DMX
 * voices sum inside the mixer, and attenuating each by 200/255 keeps that sum off the ceiling.  NIGHT's sounds sit in
 * 8 blocks and are typically one or two at a time, and the price of that headroom is every sound 2.1 dB quieter than
 * the samples it was recorded at.  255 means "the samples ARE the level"; the menu's SFX Volume is what turns it down,
 * not this constant. */
static int      g_sfx_volume = 255;   /* per-voice: unity, so the menu's SFX
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
    /* The mixer's own output stage.  Nothing in this port ever set it, so every volume below was multiplied by an OS
     * default nobody had measured -- openfpgaOS-Doom sets 255 explicitly (i_sdlsound.c:155), which is the tell that a
     * core may not assume the default.  night.ini MASTER_VOL= keeps it open for a measurement. */
    of_mixer_set_master_volume(of_setting_master_vol());
    of_cdda_set_gain(of_setting_cdda_gain());     /* sample gain before the mixer, docs/361 */
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
        if (of_setting_sfx_gain() == 100) {
            memcpy(g_sfx_pool[b], g_sfx_scratch, (size_t)got * sizeof(int16_t));
        } else {
            /* The game's own samples are the quiet part of this port: the loudest one the shipped driver ever put on
             * the board reached 30720, and the bulk of the material is far below that.  Gain belongs here, on the way
             * into the mixer's pool, because the mixer's own volume knobs top out at "no attenuation" -- 255 means
             * "the samples are the level", and the samples are what is too quiet (docs/361). */
            for (uint32_t i = 0; i < got; i++)
                g_sfx_pool[b][i] = of_gain16(g_sfx_scratch[i], of_setting_sfx_gain());
        }
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
/* task-9 (docs/364): the CDDA pump used to run on EVERY spin visit.  The instrument counted 295 921 377
 * of_cdda_pump visits in a 1964-frame desktop run -- about 150 000 per frame, 0.1 % of which had anything to
 * do -- because the game's own spin loops re-enter the pump (fn_00035ec2 and friends call sys_port_spin).
 * Everything the pump decides is a function of elapsed time (g_budget += elapsed*75 sectors) and its finest
 * action is one 12-sector chunk = 160 ms of audio, in a ring that holds 4 s.  Locking it to the game's own
 * 490 Hz tick is 320x coarser than that cadence and 300 000x cheaper than the spin rate, and it costs no
 * clock read: g_timer_calls is already in the pump.  The pre-fill, the retry-after-failure and the budget
 * arithmetic are all time-based, so none of them depends on visit frequency. */
static uint32_t g_cdda_tick_seen;
static void cdda_pump(void) {
    if (!cdda_allowed()) return;
    if (g_cdda_tick_seen == g_timer_calls) return;      /* once per tick, not once per spin */
    g_cdda_tick_seen = g_timer_calls;
    of_cdda_pump();
}
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
#define PERIPH_LOG_LIMIT 160000             /* 0.1.41 stopped a 90 s run at 90 KB; the slot is 256 KB and the scan
                                             * is what the run is for, so it gets more room but not all of it */

/* 0.1.41's log narrowed the search to two words out of 96.  Filtering the live moves down to registers whose values
 * stayed inside the small set a menu can produce left exactly:
 *   0x4000006c  six changes, every one inside 1..4, monotone with wrap: 2 -> 3 -> 4 -> 1 -> 2 -> 3 -> 4, each held
 *               9-19 s -- the shape of a list being advanced one option at a time while the player held it.
 *   0x40000008  two changes, 0 -> 1 -> 2.
 * Everything else that moved was chatter (0x40000004 is a free-running counter, 0x400000c0/dc wander over hundreds of
 * values, 0x400000e4/e8/ec are the core's own framebuffer/scaler state).
 * interactdemo names 0x4000006c MOUSE_SPEED_PCT, which is either a coincidence or an openfpgaOS variable slot the
 * interact page shares -- the persist file on the card proves the UI held the probe (id 120 val 3), not whether the
 * register followed it.  So both words are watched by name, next to the three words that would mirror the APF
 * control page (0x84/0x88/0x8c: 0x8c sat at 1 all run, which is the probe's defaultval). */
#define OF_PERIPH_CAND_A   0x4000006cu
#define OF_PERIPH_CAND_B   0x40000008u
#define OF_PERIPH_MIRROR4  0x40000084u
#define OF_PERIPH_MIRROR8  0x40000088u
#define OF_PERIPH_MIRRORC  0x4000008cu
static uint32_t g_named_prev[9];
static const struct { uint32_t addr; const char *what; } g_named[9] = {
    {OF_PERIPH_CAND_A,  "menu slot 6c"}, {OF_PERIPH_CAND_B,  "menu slot 68"},
    {OF_PERIPH_LCD,     "Pocket LCD"},  {OF_PERIPH_MIRROR4, "mirror +0x04"},
    {OF_PERIPH_MIRROR8, "mirror +0x08"}, {OF_PERIPH_MIRRORC, "mirror +0x0c"},
    {OF_PERIPH_FB_SIZE, "applied FB"},  {OF_PERIPH_FB_WIDTH, "applied stride"},
    {OF_PERIPH_SCALER,  "scaler slot"},
};

/* A candidate is a register that moved at least twice AND whose every value stayed inside the range a probe can
 * write.  Chatter registers cover the whole 32-bit space; a mirror of a menu can only ever show the menu's values.
 * Two probes with disjoint sets (A = 1..4, B = 5..8) makes the census line a verdict rather than a hint. */
static struct { unsigned moves, lo, hi; } g_cand[OF_PERIPH_WORDS];
static unsigned g_cand_tick;

static uint32_t periph_word(uint32_t addr) { return *(volatile uint32_t *)addr; }
static unsigned periph_lcd_field(void) { return (g_periph_lcd >> 16) & 3u; }   /* 0 On, 1 Off, 2 Terminal */

static void periph_named(void) {
    for (int i = 0; i < 9; i++) {
        uint32_t v = periph_word(g_named[i].addr);
        if (v == g_named_prev[i]) continue;
        uint64_t t = sys_now_us();
        NLOG("named t=%u.%us: %s @0x%08x = %08x (was %08x)\n",
             (unsigned)(t / 1000000ull), (unsigned)((t / 100000ull) % 10ull), g_named[i].what,
             g_named[i].addr, (unsigned)v, (unsigned)g_named_prev[i]);
        g_named_prev[i] = v;
        if (g_named[i].addr == OF_PERIPH_LCD) g_periph_lcd = v;          /* SCREEN=-1 follows this one */
        else if (g_named[i].addr == OF_PERIPH_FB_SIZE) g_periph_fbsize = v;
        else if (g_named[i].addr == OF_PERIPH_FB_WIDTH) g_periph_fbw = v;
        else if (g_named[i].addr == OF_PERIPH_SCALER) g_periph_scaler = v;
    }
}

static void periph_scan(int full);

/* ---- the menu channel 0.1.42 proved exists (docs/352) -----------------------------------
 * Two entries with disjoint value sets were declared and moved on hardware.  Two adjacent words of openfpgaOS's
 * peripheral page answered, each carrying only its own set, each ending exactly where the player left it:
 *
 *   0x4000006c   0 until the player moved the entry, then 1 -> 2 -> 3 -> 4; values 1..4 only, ending on 4 (High).
 *   0x40000068   stayed 0 through every moment A moved; then 0 -> 5 -> 8 -> 5 -> 8; values 5..8 only, ending on 8.
 *
 * 0.1.43 ran the same two addresses with a DIFFERENT value band (Music Volume 0x41..0x48, Screen Fit 0x51..0x53) and
 * the same two words answered again, carrying the new wire values exactly:
 *
 *   0x4000006c   0 -> 0x47 (7/8, chosen) -> 0x41 (Off) -> 0x48 (Full) -> 0x41 (Off, the value left on the menu)
 *   0x40000068   0 -> 0x52 (Stretch) -> 0x53 (Scaler) -> 0x52 -> 0x51 (Letterbox)
 *
 * Two corrections came out of that.  (1) The word is ZERO until the player actually changes the entry -- it is not
 * filled with defaultval at boot, which is what docs/353 believed from 0.1.41's log; 0.1.41's "1 at boot" was the
 * player having already moved the entry before the first sample at 9.4 s.  (2) A slot may therefore never speak at
 * all in a run, and the value 0 means "the menu has not been touched", not "the option called 0" -- which is why none
 * of the option values are 0.
 *
 * Separated in time, separated in range, and independently confirmed by the firmware's own
 * Settings/iamghost.night/Interact/_core/interact_persist.json, which stores the chosen option INDEX (3 = High for
 * id 120, 3 = Eight for id 121).  Nothing else in 96 words did that.  So a declared interact value DOES reach a
 * register an app may read -- it just never came through of_interact_get(), and it never came through the control
 * word, which is unreadable (0.1.37) and fatal to mount on (0.1.27/0.1.34).
 *
 * The layout rule is STILL unknown: 0xF700000C lands at 0x4000006c and 0xF7000010 at 0x40000068, which is neither
 * declaration order nor id order nor an offset of the declared address.  So the port does two things: it asserts the
 * two words hardware measured twice, and it verifies them -- a slot is only trusted while every value it ever shows
 * is 0 or one of that entry's own option values.  A register that shows anything else was never the slot.  0.1.43
 * showed why the verification is not paranoia: the counters at 0x4000010c and 0x40000150 climb through 0x41, 0x42,
 * 0x52 and 0x53 on their way up, and a claim rule that only asked "is the value in range?" would have grabbed one of
 * them and called it Music Volume. */
/* The channel map, measured three times over (docs/352, 354, 355, 356): 0xF700000C is mirrored into 0x4000006c and
 * 0xF7000010 into 0x40000068, and 0xF7000014 into nothing at all.  0.1.45 settled which rule the OS follows by
 * declaring the entries out of address order -- Screen Fit, declared LAST at 0xF7000010, was mirrored; SFX Volume,
 * declared SECOND at 0xF7000014, was not.  The mirror follows the ADDRESS.  Two words, no more. */
/* The menu's whole geometry arrives from interact.json through the generated header: the field an entry writes
 * (shift and mask derived from the mask it declared), the option values inside that field, and the peripheral word
 * hardware was measured carrying that bridge address -- 0 when no word has ever been seen to carry it.  main_of.c
 * deliberately keeps none of these numbers itself.  Both the 0.1.44 fault (a register that looked like a mirror) and
 * the 0.1.47 fault (inverted masks) were "the C and the JSON disagree", and generating the C from the JSON retires
 * that class of bug.
 *
 * What main_of.c does keep is the measurement a JSON cannot express: only 0xF700000C and 0xF7000010 are mirrored into
 * readable words at all (docs/356), and an entry whose mirror is known is read there and nowhere else (docs/358 §3).
 *
 * 0.1.50's open question is whether one word can hold three settings.  openfpgaOS's own interact.json packs SIX
 * entries into 0xF7000000 with disjoint masks -- Enable Analogizer bit 15, SNAC Adapter bits 0-4, SNAC Controller
 * Assignment bits 6-9, Analogizer Video Out bits 10-13, 15kHz Timing bits 18-19, Pocket LCD bits 16-17 -- so packing
 * several elements into one address is the documented shape of a Core Options word, not an invention of ours.  What
 * 0.1.47 measured was the OS writing the whole word, and docs/345's spec says why that is the expected result for a
 * `writeonly` element: the read-modify-write cycle "requires the core to have both read and write support at each
 * desired address, unless the element is flagged as writeonly", and an element with no read-back has nothing to
 * compose into.  0.1.50 drops writeonly and packs music (bits 0-3), sfx (bits 4-7) and fit (bits 8-9) into
 * 0xF700000C.  If the mirrored word holds music|sfx|fit at once, the read-back works on an undecoded address,
 * packing is alive and 0xF7000010 frees up for a fourth setting.  If it holds one value at a time, the interact read
 * is as dead as of_interact_get() looks (docs/342) and mk_dist's LAYOUT goes back to "proven". */
/* No fallback constants with the same names as the generated ones: 0.1.50's first attempt defined OF_FIELD_FIT_SHIFT
 * twice (the inert default and the real one) and the checker read whichever came first in the file -- an argument with
 * itself.  An undeclared entry simply has no row, and a row that is absent is inert by construction. */
enum { MENU_MUSIC, MENU_FIT, MENU_SFX, MENU_N };
struct menu_var {
    const char *what;
    int idx;                     /* the generated interact index, kept only for the cross-check log */
    int kind;                    /* which setting this value drives */
    unsigned shift, mask;        /* this entry's FIELD inside the shared word */
    unsigned lo, hi, dflt;       /* option values INSIDE that field (never 0: 0 means "not spoken yet") */
    uint32_t known;              /* the peripheral word hardware was seen to carry it, 0 = unknown */
    uint32_t slot;               /* the word currently trusted, 0 = nothing claimed yet */
    unsigned last;               /* last FIELD value read from that slot; 0 = the menu has not spoken */
};
static struct menu_var g_menu[MENU_N] = {
    {"Music Volume", OF_IDX_MUSIC, MENU_MUSIC, OF_FIELD_MUSIC_SHIFT, OF_FIELD_MUSIC_MASK,
     OF_FIELD_MUSIC_LO, OF_FIELD_MUSIC_HI, OF_FIELD_MUSIC_DFLT, OF_MENU_SLOT_MUSIC, 0u, 0u},
    {"SFX Volume",   OF_IDX_SFX,   MENU_SFX,   OF_FIELD_SFX_SHIFT,   OF_FIELD_SFX_MASK,
     OF_FIELD_SFX_LO,   OF_FIELD_SFX_HI,   OF_FIELD_SFX_DFLT,   OF_MENU_SLOT_SFX,   0u, 0u},
    /* Screen Fit is live exactly when interact.json declares it (LAYOUT in mk_dist picks which layout that is).
     * Inert otherwise: idx < 0 skips the scan, so a row with no entry can never hunt for a word and mistake a
     * counter for a menu that does not exist.  Its pad route -- L1+B cycles letterbox / stretch / scaler -- works in
     * both layouts, and a live menu field does not override it: an apply only fires when the FIELD value changes, so
     * the combo is free until the player moves the menu entry. */
#ifdef OF_IDX_FIT                                /* mk_dist's LAYOUT decides whether Screen Fit is declared at all */
    {"Screen Fit",   OF_IDX_FIT,   MENU_FIT,   OF_FIELD_FIT_SHIFT,   OF_FIELD_FIT_MASK,
     OF_FIELD_FIT_LO,   OF_FIELD_FIT_HI,   OF_FIELD_FIT_DFLT,   OF_MENU_SLOT_FIT,   0u, 0u},
#endif
};

/* Per-register trust, measured against every entry: a word is clean for an entry if EVERY sample it has ever shown
 * is either 0 or one of that entry's option values, and it has been seen holding a real option value more than once.
 * A counter climbing through the range fails the first test; a word that answers the menu passes both. */
static uint8_t g_menu_clean[MENU_N][OF_PERIPH_WORDS];
static uint16_t g_menu_seen[MENU_N][OF_PERIPH_WORDS];
static uint8_t g_menu_zero[MENU_N][OF_PERIPH_WORDS];    /* was this word ever seen silent? */
static uint8_t g_menu_mute_warn[MENU_N];                /* say "no slot" once, not every census */
static uint8_t g_menu_init;
#define MENU_SEEN_MIN 5u                          /* 5 samples of settings_beat = half a second holding a value */

static unsigned menu_field(const struct menu_var *m, uint32_t v) { return (v >> m->shift) & m->mask; }

/* Every test below is a test on this entry's FIELD, not on the whole word: three entries read one word, and what one
 * of them writes says nothing about the others. */
static int menu_opt(const struct menu_var *m, uint32_t v) {
    unsigned f = menu_field(m, v);
    return f == 0u || (f >= m->lo && f <= m->hi);
}
static int menu_is_option(const struct menu_var *m, uint32_t v) {
    unsigned f = menu_field(m, v);
    return f >= m->lo && f <= m->hi;
}

static const char *menu_option_name(const struct menu_var *m, unsigned f) {
    static const char *vol[] = {"Off", "1/8", "1/4", "1/2", "5/8", "3/4", "7/8", "Full"};
    static const char *fit[] = {"letterbox", "stretch", "scaler 320x200"};
    unsigned k = f - m->lo;                                      /* option index inside this entry's own band */
    if (m->kind == MENU_MUSIC || m->kind == MENU_SFX) return (k <= 7u) ? vol[k] : "?";
    return (k <= 2u) ? fit[k] : "?";
}

static int menu_apply(struct menu_var *m, unsigned f) {
    unsigned k = f - m->lo;
    if (m->kind == MENU_MUSIC) return of_setting_apply_music_vol((int)k * 255 / 7);              /* 0..7 -> 0..255 */
    if (m->kind == MENU_SFX)   return of_setting_apply_sfx_vol((int)k * 255 / 7);
    return of_setting_apply_fit((int)k);
}

static unsigned g_menu_beat;
static void menu_sample(void) {
    if (!g_menu_init) {
        for (int k = 0; k < MENU_N; k++)
            for (int i = 0; i < OF_PERIPH_WORDS; i++) g_menu_clean[k][i] = 1;
        g_menu_init = 1;
    }
    /* A full 96-word pass costs 96 peripheral reads.  settings_beat runs every 100 ms, and a 640x480 present already
     * costs 21 ms of the 33 ms frame (docs/352), so the sweep runs at 2 Hz while a slot is still unclaimed and stops
     * entirely once every slot is claimed -- after that a beat is one read per setting. */
    int all_claimed = 1;
    for (int k = 0; k < MENU_N; k++) if (!g_menu[k].slot) all_claimed = 0;
    if (all_claimed || (g_menu_beat++ % 5u)) return;
    for (int i = 0; i < OF_PERIPH_WORDS; i++) {
        uint32_t v = periph_word(OF_PERIPH_BASE + 4u * i);
        for (int k = 0; k < MENU_N; k++) {
            if (!menu_opt(&g_menu[k], v)) { g_menu_clean[k][i] = 0; continue; }
            if (menu_field(&g_menu[k], v) == 0u) g_menu_zero[k][i] = 1;         /* docs/354: a mirror word is silent until the player moves
                                                         * its entry, so a word that was never silent is not one.
                                                         * 0.1.44 learned this the hard way: 0x4000003c sits at 0x63
                                                         * from boot, which is inside SFX Volume's option band, holds
                                                         * it for the entire run, and is not the SFX mirror -- it is a
                                                         * peripheral constant that happens to look like "1/4".  Range
                                                         * cleanliness plus "held an option value" cannot tell a
                                                         * constant mirror from a constant register; only the silence
                                                         * before the first word can. */
            if (menu_is_option(&g_menu[k], v) && g_menu_seen[k][i] < 60000u) g_menu_seen[k][i]++;
        }
    }
}

static void menu_claim(struct menu_var *m, uint32_t a, uint32_t v, const char *how) {
    m->slot = a; m->last = menu_field(m, v);
    uint64_t t = sys_now_us();
    NLOG("menu: %s slot CLAIMED t=%u.%us @0x%08x %s (word %08x, field %u = %s at bits %u-%u)\n",
         m->what, (unsigned)(t / 1000000ull), (unsigned)((t / 100000ull) % 10ull), a, how, (unsigned)v, m->last,
         menu_option_name(m, m->last), m->shift, m->shift + 31u - (unsigned)__builtin_clz(m->mask));
}

static int menu_slots_scan(void) {
    int ch = 0;
    menu_sample();
    for (int k = 0; k < MENU_N; k++) {
        struct menu_var *m = &g_menu[k];
        if (!m->what) continue;                   /* a row the layout does not declare: nothing to read, and a
                                                   * zero-initialised hole must never be treated as an entry */

        if (m->slot && !menu_opt(m, periph_word(m->slot))) {      /* a word whose FIELD shows what the menu cannot
                                                                    * write was never the slot */
            NLOG("menu: %s slot @0x%08x lost (field %u is neither 0 nor an option %u..%u of word %08x)\n",
                 m->what, m->slot, menu_field(m, periph_word(m->slot)), m->lo, m->hi,
                 (unsigned)periph_word(m->slot));
            m->slot = 0; m->last = 0;
        }
        if (!m->slot && m->known && menu_is_option(m, periph_word(m->known)))
            menu_claim(m, m->known, periph_word(m->known), "the word hardware carried it in on");
        if (!m->slot && m->known) continue;     /* docs/358: an entry whose mirror address is known may ONLY be read
                                                 * there.  0.1.47's fallback hunt claimed 0x400000ec for Music Volume
                                                 * and 0x400000dc for SFX Volume -- two peripheral counters whose low
                                                 * nibble wandered through the option band and which had been 0 at
                                                 * boot, so they passed every test the hunt had.  Music Volume was then
                                                 * driven from a counter: field 7 -> 218, field 1 -> Off (mute), field 0
                                                 * -> back to night.ini, i.e. "it got muted whatever I set".  A known
                                                 * address that never answers is reported, not replaced by a guess. */
        if (!m->slot) {
            for (int i = 0; i < OF_PERIPH_WORDS; i++) {           /* only for entries whose address is unknown */
                uint32_t a = OF_PERIPH_BASE + 4u * i, v = periph_word(a);
                if (!g_menu_clean[k][i] || !g_menu_zero[k][i] || g_menu_seen[k][i] < MENU_SEEN_MIN
                    || !menu_is_option(m, v)) continue;
                /* Sharing a word is the point of packing: three entries may claim the SAME word because they read
                 * disjoint fields of it, so the old "somebody else claimed it" veto is gone. */
                menu_claim(m, a, v, "found by field cleanliness");
                break;
            }
        }
        if (!m->slot) {
            if (!g_menu_mute_warn[k] && sys_now_us() > 60000000ull) {   /* this OS mirrors only some of the app's
                                                                        * entries; the rest stay on night.ini, and the
                                                                        * port now says so out loud instead of leaving
                                                                        * a menu control that quietly does nothing */
                g_menu_mute_warn[k] = 1;
                NLOG("menu: %s has no peripheral slot after 60 s -- this OS mirrors only some of the app's entries"
                     " and the value from night.ini stands\n", m->what);
            }
            continue;
        }

        uint32_t w = periph_word(m->slot);
        unsigned f = menu_field(m, w);
        if (f == 0u) {                                            /* this FIELD is silent: the night.ini value stands,
                                                                  * whatever the other entries wrote into the same word */
            if (m->last != 0u) {
                m->last = 0;
                NLOG("menu: %s field in slot @0x%08x reads 0 again -- the value from night.ini stands\n",
                     m->what, m->slot);
            }
            continue;
        }
        if (f != m->last) {
            m->last = f;
            ch |= menu_apply(m, f);
            NLOG("menu: %s = %s (field %u of word %08x) applied: %s\n", m->what, menu_option_name(m, f), f,
                 (unsigned)w, m->kind == MENU_MUSIC ? "music volume" : m->kind == MENU_SFX ? "sound-effect volume"
                 : "panel fit");
        }
    }
    return ch;
}

static void periph_scan(int full) {
    uint64_t tstamp = sys_now_us();
    if (++g_cand_tick % 30u == 0u) {            /* the verdict, written by the core.  0.1.43's version of this line
                                                 * claimed 45 candidates because the range test compared a signed int
                                                 * against a free-running counter above 2^31 -- a 32-bit lesson, and
                                                 * the reason the test is done on the same cleanliness rule the claim
                                                 * uses instead of on a min/max range. */
        int n = 0;
        for (int i = 0; i < OF_PERIPH_WORDS; i++)
            for (int k = 0; k < MENU_N; k++)
                if (g_menu_clean[k][i] && g_menu_zero[k][i] && g_menu_seen[k][i] >= MENU_SEEN_MIN) {
                    n++;
                    NLOG("cand @0x%08x: clean for %s, %u samples holding an option value\n",
                         OF_PERIPH_BASE + 4u * i, g_menu[k].what, (unsigned)g_menu_seen[k][i]);
                    break;
                }
        /* Named from the table, not hardcoded: 0.1.48 reordered g_menu (Music, SFX, Fit) and this line went on
         * reporting "Music Volume, Screen Fit" for slots 0 and 1, which is how a log can describe a core that no
         * longer exists. */
        char claimed[160];
        int clen = 0;
        for (int k = 0; k < MENU_N; k++)
            clen += snprintf(claimed + clen, sizeof(claimed) - clen, "%s%s%s", k ? ", " : "", g_menu[k].what,
                             g_menu[k].slot ? "" : "(none)");
        NLOG("census t=%u.%us: %d candidate(s) of %d registers; claimed: %s\n",
             (unsigned)(sys_now_us() / 1000000ull), (unsigned)((sys_now_us() / 100000ull) % 10ull),
             n, OF_PERIPH_WORDS, claimed);
    }
    for (int i = 0; i < OF_PERIPH_WORDS; i++) {
        uint32_t v = *(volatile uint32_t *)(OF_PERIPH_BASE + 4u * i);
        if (full && (i % 8) == 0) {
            char acc[128]; int n = 0;
            for (int j = i; j < i + 8 && j < OF_PERIPH_WORDS; j++)
                n += snprintf(acc + n, sizeof acc - (size_t)n, " %08x",
                              (unsigned)*(volatile uint32_t *)(OF_PERIPH_BASE + 4u * j));
            NLOG("periph t=%u.%us %04x:%s\n", (unsigned)(tstamp / 1000000ull),
                 (unsigned)((tstamp / 100000ull) % 10ull), 4u * i, acc);
        }
        if (g_periph_prev[i] != v) {
            /* Stamped with wall time on purpose (docs/350): correlating a menu change with a register needs the
             * move pinned in time, so "I moved Video H Offset at about 40 s" becomes a readable log line. */
            uint64_t t = sys_now_us();
            NLOG("periph move%s @0x%08x t=%u.%us: %08x -> %08x\n", (v <= 8u) ? " [small]" : "",
                 OF_PERIPH_BASE + 4u * i,
                 (unsigned)(t / 1000000ull), (unsigned)((t / 100000ull) % 10ull),
                 (unsigned)g_periph_prev[i], (unsigned)v);
            if (!g_cand[i].moves) g_cand[i].lo = g_cand[i].hi = v;
            else {
                if (v < g_cand[i].lo) g_cand[i].lo = v;
                if (v > g_cand[i].hi) g_cand[i].hi = v;
            }
            g_cand[i].moves++;
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
    /* of_settings_poll() is empty because the control word is unreadable (0.1.37) and the interact ecall answers a
     * fixed table of junk; these registers are the live path 0.1.42 proved exists (docs/352). */
    int ch = of_settings_poll();
#ifndef OF_PC
    ch |= menu_slots_scan();        /* the peripheral slots: the only live settings path this OS has (docs/352) */
#endif
    if (ch & OF_SET_VOL_SEEN)
        NLOG("settings: music raw=%08x vol=%d\n",
             (unsigned)of_setting_music_bridge(), of_setting_music_vol());
    if (ch & OF_SET_VOL) {
        of_cdda_set_volume(of_setting_music_vol());
        of_mixer_set_group_volume(OF_MIXER_GROUP_SFX, of_setting_sfx_vol() & 0xff);
        /* Every stage in one line, because "it is quiet" has four possible causes and the log should name them. */
        NLOG("settings: music_vol=%d applied from the menu | mixer master=%d music_voice=%d sfx_group=%d sfx_voice=%d"
             " | gain sfx=%d%% cdda=%d%%\n",
             of_setting_music_vol(), of_setting_master_vol(), of_setting_music_vol(),
             of_setting_sfx_vol(), g_sfx_volume, of_setting_sfx_gain(), of_setting_cdda_gain());
    }
    if (ch & OF_SET_LCD)
        NLOG("settings: pocket lcd raw=%08x scr=%d\n", of_setting_lcd_raw(), of_setting_screen());
    if (ch & OF_SET_SFX) {
        of_mixer_set_group_volume(OF_MIXER_GROUP_SFX, of_setting_sfx_vol() & 0xff);
        NLOG("settings: sfx_vol=%d applied from the menu\n", of_setting_sfx_vol());
    }
    if (ch & OF_SET_FIT) {
        NLOG("settings: fit -> %d (from the peripheral slot, not the bridge)\n", of_setting_fit());
        fit_apply("menu");
    }
}

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

/* L1 held + Select saves the current slot, L1 + Start loads it, L1 + A cycles to the next slot, L1 + B cycles
 * the panel fit (letterbox -> stretch -> scaler).
 * L1 is F1 to the game and Select/Start are Esc/Enter, so the combo also reaches the shipped key state --
 * that is unavoidable on 11 buttons and is why the game sees a pause-menu key press on a save. */
static void pad_combo(void) {
    if (!pad_script_held(OF_BTN_L1) && !of_btn(OF_BTN_L1)) return;
    static uint8_t seen_select, seen_start, seen_a, seen_b;
    int s = pad_script_held(OF_BTN_SELECT) || of_btn(OF_BTN_SELECT);
    int st = pad_script_held(OF_BTN_START) || of_btn(OF_BTN_START);
    int a = pad_script_held(OF_BTN_A) || of_btn(OF_BTN_A);
    int b = pad_script_held(OF_BTN_B) || of_btn(OF_BTN_B);
    if (s && !seen_select) { seen_select = 1; if (of_save_write(of_save_slot()) != 0 && of_save_last_error()) printf("save: %s\n", of_save_last_error()); }
    if (!s) seen_select = 0;
    if (st && !seen_start) { seen_start = 1; if (of_save_read(of_save_slot()) == 0) g_load_slot = of_save_slot(); else if (of_save_last_error()) printf("load: %s\n", of_save_last_error()); }
    if (!st) seen_start = 0;
    if (a && !seen_a) { seen_a = 1; of_save_slot_next(); }
    if (!a) seen_a = 0;
    if (b && !seen_b) {
        seen_b = 1;
        NLOG("fit: pad cycle -> %d (0 letterbox, 1 stretch, 2 scaler)\n", of_setting_fit_cycle());
        fit_apply("pad");
    }
    if (!b) seen_b = 0;
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

    settings_beat(now);
    music_mode_mirror();

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

    /* Re-anchoring on a lag of eight periods (16 ms) is what put the logo back into slow motion: a present
     * that costs 81 ms re-anchored every single frame, so the game received one tick per frame -- 13 ticks a
     * second instead of 490 -- and a fade measured in ticks never reached its end (reanchor=130 with
     * frames=130).  The lag here is OUR blit, not the game blocking, and the right answer is to hand over the
     * ticks that actually elapsed: 40 in a 81 ms frame, which is what a 12 fps renderer still owes the clock.
     * Only a lag beyond half a second is treated as a real stall (a blocking read, a clock discontinuity) and
     * re-anchored instead, and the burst cap goes to 128 so a 250 ms stall is absorbed. */
    unsigned calls = 1;
    if (now - g_tick_next_us > 500000ull) {           /* a genuine stall: re-anchor, don't burst */
        g_tick_next_us = now;
        g_pump_reanchor++;                            /* a stall inside game code shows up here */
    } else {
        while (now >= g_tick_next_us && calls < 128u) { g_tick_next_us += period; calls++; }
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
        g_present_us_sum += d; g_present_n++;
        g_frames++;
#ifdef OF_PC
        /* The rig's state witness on the same grid as its screen witness.  audio_bytes is the Red Book the host
         * has actually delivered (2352 bytes a sector), the same quantity src/app/main.c hands the sink. */
        if (g_rig_dir) rig_poll((long)(now / 1000ull), (unsigned long long)g_frames,
                                (unsigned long long)of_cdda_pos() * 2352ull);
#endif
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
#ifndef OF_PC
            if (g_view_320x200) geo_witness("view");      /* only while the 320x200 picture is up: that is where
                                                              * 0.1.29 lost the mode, and this line is the witness */
            {   /* A second sample of the same interact table, three seconds into the run.  A real menu value is
                 * stable and moves when the knob moves; an unpopulated table is garbage that does not care.  The
                 * 0.1.28 run read 0x55555555 everywhere and the 0.1.29 run read a different garbage per index --
                 * neither behaviour distinguishes those two, but a value that HOLDS and then CHANGES does. */
                static unsigned probe_tick;
                static int interact2_done;
                if (of_setting_diag() & OF_DIAG_PERIPH) {
                    periph_named();                 /* four registers, by name: cheap, proven safe, and the only
                                                     * menu state this OS has ever been seen to publish */
                    if (g_periph_watch) {
                        long lb = of_log_bytes();
                        if (lb > PERIPH_LOG_LIMIT) {
                            g_periph_watch = 0;
                            NLOG("periph: the 96-word watch stopped at %ld log bytes (the save slot is 256 KB and"
                                 " the game's own lines outrank a scan).  The named registers keep running\n", lb);
                        } else {
                            periph_scan(probe_tick && (probe_tick % 20u) == 0u ? 1 : 0);
                        }
                    }
                }
                probe_tick++;
                if (probe_tick == 3u && !interact2_done && (of_setting_diag() & OF_DIAG_INTERACT)) {
                    interact2_done = 1;
                    for (int i = 0; i < OF_INTERACT_COUNT; i++) {
                        unsigned v = 0;
                        long e = of_interact_probe(i, &v);
                        NLOG("interact2[%d]: err=%ld val=%08x\n", i, e, v);
                    }
                }
            }
#endif
            NLOG("perf pres=%uus max=%uus pump=%u ticks=%u skips=%u\n",
                 g_present_us, g_present_us_max, g_pump_calls, g_timer_calls, g_present_skips);
            NLOG("stages last m=%u p=%u c=%u f=%u x=%u | max all=%u copy=%u flip=%u\n",
                 g_lus_mode, g_lus_pal, g_lus_copy, g_lus_flush, g_lus_flip,
                 g_us_flip_max, g_us_copy, g_us_flip);

            /* task-9a: the whole frame budget on one line -- what presenting costs, what streaming the disc
             * costs, and how close the mixer ever came to running out of audio.  The disc half is the number
             * that decides whether an async read or a deeper ring is worth building. */
            {
                uint32_t d_r = 0, d_max = 0, d_by = 0, d_sk = 0, d_fl = 0;
                uint32_t c_f = 0, c_max = 0, c_fr = 0, c_low = 0, c_hold = 0, c_behind = 0, c_am = 0, c_pv = 0, c_dry = 0;
                uint64_t d_us = 0, c_us = 0;
                of_disc_stats(&d_r, &d_us, &d_max, &d_by, &d_sk, &d_fl);
                of_cdda_stats(&c_f, &c_us, &c_max, &c_fr, &c_low, &c_hold, &c_behind, &c_am, &c_pv, &c_dry);
                NLOG("budget present n=%u mean=%uus max=%uus | disc reads=%u mean=%uus max=%uus seeks=%u fails=%u kb=%u"
                     " | cdda fills=%u mean=%uus max=%uus frames=%u visits=%u audio_low=%u room_hold=%u"
                     " clock_behind=%u dry=%u ahead_min=%u(%.2fs)\n",
                     g_present_n, g_present_n ? (unsigned)(g_present_us_sum / g_present_n) : 0u, g_present_us_max,
                     d_r, d_r ? (unsigned)(d_us / d_r) : 0u, d_max, d_sk, d_fl, d_by / 1024u,
                     c_f, c_f ? (unsigned)(c_us / c_f) : 0u, c_max, c_fr, c_pv, c_low, c_hold, c_behind, c_dry,
                     c_am, c_am / 44100.0);
            }
#ifndef OF_PC
            NLOG("pump reanchor=%u frames=%u hz=%u log=%d/%ld dm=%d\n",
                 g_pump_reanchor, g_frames, hz, g_log_ok, of_log_bytes(), g_display_mode);
            NLOG("menu scr=%d blit fast=%u slow=%u skip=%u\n",
                 of_setting_screen(), g_ds_fast, g_ds_slow, g_present_skips);
            /* Once a second, not once a frame: the mode is asserted at boot, but fn_03f15f (the game's own VESA
             * mode set) runs through the boot sequence and the display path is not obliged to remember what the
             * app asked for.  1 Hz keeps the assertion without the 59 Hz cost a per-frame one adds. */
            /* SCREEN=-1 means "follow the core's own Pocket LCD", and that is only a promise if the value can be
             * read.  It can: register 0x40000080 carries the menu choice (0.1.38), and this is the only place the
             * port acts on it.  With the periph probe off (DIAG=0, the shipping value) -1 behaves as 0, which is
             * exactly what it has always done. */
            if (of_setting_screen() < 0 && g_periph_lcd)
                g_display_mode = (periph_lcd_field() == 2u) ? OF_DISPLAY_TERMINAL : OF_DISPLAY_FRAMEBUFFER;
            of_video_set_display_mode(g_display_mode);
            g_display_applied = g_display_mode;
#else
            NLOG("pump reanchor=%u frames=%u hz=%u log=%d/%ld\n",
                 g_pump_reanchor, g_frames, hz, g_log_ok, of_log_bytes());
#endif
            NLOG("audio cdda=%u/%u/%u plays=%u stops=%u sfx=%u/%u/%u ring=%u head=%d music_mode=%u\n",
                 of_cdda_pos(), of_cdda_sectors(), of_cdda_tracks(), sys_cd_audio_play_calls(),
                 sys_cd_audio_stop_calls(), g_of_sfx_plays, g_of_sfx_drops, g_of_sfx_steals,
                 of_cdda_ring_ahead(), (int)of_cdda_head(), (unsigned)g_dat_51a10);
            if (sys_audio_dsp_unknown_total()) {
                /* Name them.  A total only says something was ignored; which byte it was says whether the
                 * missing sound is an arm the port never acts on. */
                char acc[96]; int n = 0;
                acc[0] = 0;
                int shown = 0;
                for (int c = 0; c < 256 && shown < 6; c++) {
                    uint32_t k = sys_audio_dsp_unknown((uint8_t)c);
                    if (!k) continue;
                    n += snprintf(acc + n, sizeof acc - (size_t)n, " 0x%02x=%u", c, k);
                    shown++;
                }
                NLOG("audio: dsp commands with no case in the port = %u:%s\n",
                     sys_audio_dsp_unknown_total(), acc);
            }
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


#ifndef OF_PC
/* What the memory system actually gives us, measured at boot instead of argued about.  The device log reports
 * "copy=80391us" for one 640x480 -> 320x240 pass -- 307 200 source bytes read and 76 800 written in 80 ms,
 * about 3.8 MB/s -- and that one stage is what pins the tick pump at 12 Hz.  Whether 80 ms is the floor or the
 * loop is answerable by timing the parts against each other: a plain sequential read of the same bytes, a
 * memcpy of them, a memset, and the downscale itself. */
static void mem_bench(void) {
    const size_t n = 640u * 480u;
    static uint8_t *scratch;
    if (!scratch) scratch = (uint8_t *)malloc(n);
    volatile uint32_t sink = 0;
    uint64_t t = sys_now_us();
    for (size_t i = 0; i < n; i += 4u) sink += *(const uint32_t *)(const void *)(g_video_ram + i);
    NLOG("bench read=%uus sink=%u\n", (uint32_t)(sys_now_us() - t), (unsigned)(sink & 1u));
    t = sys_now_us();
    for (size_t i = 0; i < n; i += 4u) sink += *(const uint32_t *)(const void *)(g_video_ram + i);
    NLOG("bench read2=%uus (warm)\n", (uint32_t)(sys_now_us() - t));
    if (scratch) {
        t = sys_now_us(); memcpy(scratch, g_video_ram, n);
        NLOG("bench copy=%uus\n", (uint32_t)(sys_now_us() - t));
        t = sys_now_us(); memset(scratch, 0xA5, n);
        NLOG("bench write=%uus\n", (uint32_t)(sys_now_us() - t));
        free(scratch);
    }
    t = sys_now_us();
    downscale_2x2_frame(g_video_ram, of_video_surface());
    NLOG("bench downscale=%uus\n", (uint32_t)(sys_now_us() - t));
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
        else if (!strcmp(argv[i], "--rig-hz") && i + 1 < argc) g_rig_hz = atoi(argv[++i]);
        /* Full-picture witnesses are 4 bytes a pixel; a nine-stage sweep does not need them when the arena
         * raws (1 byte a pixel, the docs/62 basis) are what the comparison reads.  0 turns them off. */
        else if (!strcmp(argv[i], "--rig-witness") && i + 1 < argc) g_rig_witness = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--warp-stage") && i + 1 < argc) g_warp_stage_cli = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--save") && i + 1 < argc) g_save_slot = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--save-at") && i + 1 < argc) g_save_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--load") && i + 1 < argc) g_load_slot = atoi(argv[++i]);
#endif
    }
    if (getenv("NIGHT_SEED")) g_rng_seed_override = atoi(getenv("NIGHT_SEED"));   /* docs/107 reproducible runs */
    /* night.ini LOG= is the switch (docs/362): 0 writes no file, 1 reopens the trail this port was built with.
     * The stdout half of NLOG stays live either way, so a desktop run and the rig keep their witness. */
    if (of_setting_log_wanted()) {
        g_log_ok = (of_log_open() == 0) ? 1 : 0;
    } else {
        g_log_ok = 0;
        printf("log: file logging off (night.ini LOG=1 writes Saves/night/common/nightlog.sav)\n");
    }
    /* The first line a running app writes, on purpose.  0.1.27 broke the core and the card kept the PREVIOUS
     * run's log, which is only interpretable because the old log's settings line said music_vol=192 -- if this
     * line is missing from a card log, the app never reached the point of writing anything, and the fault is in
     * what the firmware did before it ran (a core definition file, a slot, a bitstream), not in app code. */
    NLOG("core %s: boot marker\n", OF_CORE_VERSION);
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
#ifndef OF_PC
    /* Claim the game display at the top of boot.  0.1.35 stalled mid-boot and what the player saw was
     * openfpgaOS's terminal, "regardless of the Pocket LCD setting" -- the display mode the host asks for is only
     * asserted from the pump, and a run that never reaches the pump never asks for anything. */
    NLOG_STEP("display claimed");
    of_video_set_display_mode(OF_DISPLAY_FRAMEBUFFER);
#endif
    of_audio_init();

    const struct of_capabilities *caps = of_get_caps();
    NLOG("night pocket: cpu=%u Hz sdram=%u KB heap=%u KB fb=%ux%u stride=%u voices=%u variant=%u\n",
           caps->cpu_freq_hz, caps->sdram_size / 1024, caps->heap_size / 1024,
           caps->fb_width, caps->fb_height, caps->fb_stride, caps->mixer_voices, caps->core_variant);
    NLOG("caps: version=%u hw=%08x os=%08x (sdk OF_CAPS_VERSION=%d)\n",
         caps->version, caps->hw_features, (caps->version >= 4) ? caps->os_features : 0u, OF_CAPS_VERSION);
    of_settings_load();                        /* os.ini [night] on the device, env on the desktop */
    NLOG("settings: music_vol=%d sfx_vol=%d cdda=%d pad=%d tick_hz=%d screen=%d file_log=%d\n",
         of_setting_music_vol(), of_setting_sfx_vol(), of_setting_cdda(), of_setting_pad(),
         of_setting_tick_hz(), of_setting_screen(), g_log_ok);
    NLOG("fit=%d (0 = 1:1 with 20+20 black bars, 1 = host row stretch, 2 = ask the core scaler for 320x200)\n",
         of_setting_fit());
    NLOG("diag=0x%x (1 periph scan 0x40000000+, 2 analogizer probe, 4 interact census, 8 control-word read)"
         "\n       every probe below is gated on this; night.ini DIAG= is the switch, 0 is the shipping value\n",
         of_setting_diag());
#ifndef OF_PC
    /* The control in the interact experiment.  The RESERVED menu values have a documented route that is not
     * interact -- of_analogizer.h hands the app what the firmware wrote for Analogizer / SNAC / video offsets --
     * so if this reads coherently while of_interact_get() reads garbage, the gap is the interact ecall and not
     * the menu plumbing. */
    {   /* of_analogizer_state() reads the SAME menu values the interact.json reserved entries point at -- video
         * mode, SNAC type, H/V offsets -- through a different ecall (OF_EID_ANALOGIZER).  If it answers coherently
         * while of_interact_get() answers garbage, the gap is the interact ecall specifically. */
        if (of_setting_diag() & OF_DIAG_PERIPH) {
            NLOG_STEP("probe periph begin");      /* bracketed on purpose: the probe that faults is the one whose
                                                     * "begin" is the last line the run ever wrote */
            NLOG("periph: reading %u words at 0x%08x (the interactdemo approach)\n",
                 OF_PERIPH_WORDS, OF_PERIPH_BASE);
            periph_scan(1);
            NLOG("periph: baseline set.  Every later 'periph move' line is a live change -- move Pocket LCD in"
                 " Core Options and the word that answers is the menu channel this core has been looking for\n");
            NLOG_STEP("probe periph done");
        }
        if (of_setting_diag() & OF_DIAG_ANALOGIZER) {
            NLOG_STEP("probe analogizer begin");
            of_analogizer_state_t az;
            int arc = of_analogizer_state(&az);
            NLOG("analogizer: enabled=%d state rc=%d en=%u video=%u snac=%u/%u off=%d,%d\n",
                 of_analogizer_enabled(), arc, az.enabled, az.video_mode, az.snac_type, az.snac_assignment,
                 (int)az.h_offset, (int)az.v_offset);
            NLOG_STEP("probe analogizer done");
        }
    }
#endif
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
    if (g_rig_dir) {
        rig_screen_init(g_rig_dir, g_rig_witness);
        rig_init(g_rig_dir, g_rig_hz);
        NLOG("rig: state cells dumping to %s at %d Hz\n", g_rig_dir, g_rig_hz);
    }
#endif

#ifndef OF_PC
    /* Which index actually carries what.  of_interact_get() addresses variables by position, and the last two
     * times this was guessed wrong (docs/336), so the whole front of the table is printed rather than only the
     * entries the host hopes it needs. */
    if (of_setting_diag() & OF_DIAG_INTERACT) {
        NLOG_STEP("probe interact census begin");
        char acc[160]; int n = 0;
        for (int i = 0; i < OF_INTERACT_COUNT; i++)
            n += snprintf(acc + n, sizeof acc - (size_t)n, " %d=%08x", i, (unsigned)of_interact_get(i));
        NLOG("interact:%s\n", acc);
        /* 0.1.28's card log read 0x55555555 at all eight indices.  of_interact_get() discards the ecall's error,
         * so that number could mean "the OS answered this" or "the OS refused and this is junk" -- which is the
         * whole difference between a menu that is not wired and a menu the OS does not implement at all.  Both
         * halves get printed from here on. */
        for (int i = 0; i < OF_INTERACT_COUNT; i++) {
            unsigned v = 0;
            long e = of_interact_probe(i, &v);
            NLOG("interact[%d]: err=%ld val=%08x\n", i, e, v);
        }
        /* The SDK's own smoke test asserts of_interact_get(999) == 0 (src/apps/testdemo/test_interact.c), which
         * only means something if the handler validates its index.  Probe the boundary: if 999 and 63 read 0
         * while 0..n-1 read the same fixed garbage in every run, then the ecall IS dispatched and its table is
         * simply never filled -- a different statement from "the ecall does not exist", and the accurate one. */
        /* No out-of-range probes.  They were a reasonable idea -- the SDK's own test asserts
         * of_interact_get(999) == 0, which only means something if the handler validates the index -- but the
         * only way to find out on hardware is to ask the OS to index a table it may not size-check, and a
         * diagnostic that can fault the kernel is not a diagnostic.  0.1.34/0.1.35 read indices up to
         * OF_INTERACT_MAX_VARS-1 and 999; 0.1.32 read only the declared range and ran to completion.  The
         * boundary question is asked from the PC shim, where an OOB read hurts nobody. */
        NLOG_STEP("probe interact census done");
    }
#endif
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
        {
            of_video_caps_t cap; of_video_get_caps(&cap);
            NLOG("caps max=%ux%u stride=%u phys=%ux%u def=%ux%u bytes=%u modes=%d\n",
                 cap.max_width, cap.max_height, cap.max_stride, cap.physical_width, cap.physical_height,
                 cap.default_width, cap.default_height, cap.max_frame_bytes, of_video_get_mode_count());
            of_video_get_mode(&g_fb_base);
            int nmode = of_video_get_mode_count();
            for (int i = 0; i < nmode; i++) {
                of_video_mode_t m;
                if (of_video_get_mode_info(i, &m) != 0) continue;
                NLOG("mode %d: %ux%u stride=%u color=%u\n", i, m.width, m.height, m.stride, m.color_mode);
                if (m.width == 2 * PANEL_W && m.height == 2 * PANEL_H && m.color_mode == OF_VIDEO_MODE_8BIT) {
                    g_fb_native = m; g_fb_native.stride = 0; g_fb_native_ok = 1;
                }
                if (m.width == GAME_W && m.height == GAME_H && m.color_mode == OF_VIDEO_MODE_8BIT) {
                    g_fb_native_200 = m; g_fb_200_ok = 1;
                }
            }
            if (!g_fb_native_ok && cap.max_width >= 2 * PANEL_W && cap.max_height >= 2 * PANEL_H &&
                cap.max_frame_bytes >= (uint32_t)(2 * PANEL_W) * (2 * PANEL_H)) {
                /* Not on the list, but inside everything the core advertises it can hold.  Worth one bounded
                 * attempt: present() checks the return value and falls back to the software downscale on any
                 * refusal, so a wrong guess costs one log line and one slow frame, nothing else. */
                g_fb_native = (of_video_mode_t){ 2 * PANEL_W, 2 * PANEL_H, 0, OF_VIDEO_MODE_8BIT, 0 };
                g_fb_native_ok = 1;
                NLOG("fb native 640x480 not listed -- attempting it inside caps\n");
            }
            NLOG("fb native 640x480 8-bit %s (base %ux%u stride=%u)\n",
                 g_fb_native_ok ? "offered/attempted" : "NOT possible", g_fb_base.width, g_fb_base.height, g_fb_base.stride);
            fit_apply("boot");
            geo_witness("boot");
            if (!surface_is_base())
                NLOG("surface: the core did not land on the 320x240 base mode the host assumes\n");
        }
        NLOG("blit src=%p fb=%p al=%d/%d\n", (void *)g_video_ram, (void *)of_video_surface(),
             (int)((uintptr_t)g_video_ram & 3u), (int)((uintptr_t)of_video_surface() & 3u));
        mem_bench();
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
#ifdef OF_PC
    /* src/app/main.c installs sys_set_res_hook(reslog_add) and every RES lookup lands in markers.jsonl as
     * `res <name>` (docs/56's event anchor).  The Pocket host had the seam switched off, so its dumps carried an
     * empty markers.jsonl and the cross-host diff could not see a single resource load.  Installed only when a rig
     * dump is asked for: on the device this hook would run on the hot path of every resource lookup. */
    sys_set_res_hook(g_rig_dir ? rig_res_marker : 0);
#else
    sys_set_res_hook(0);
#endif

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
    g_setting_next_us = g_tick_next_us;
    g_music_mode = g_dat_51a10 ? 1u : 0u;             /* the shipped value is 1 (docs/133) */

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
#ifdef OF_PC
    if (g_rig_dir) rig_close();
#endif
    present();
    printf("frames presented: %u  ticks: %u  cdda tracks: %u  sfx voices: %u\n",
           g_frames, g_timer_calls, of_cdda_tracks(), g_of_sfx_plays);
    return 0;
}

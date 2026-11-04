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

#include "of.h"
#include "of_audio.h"
#include "of_cache.h"
#include "of_caps.h"
#include "of_file.h"
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
static uint64_t g_tick_next_us;
static uint64_t g_present_next_us;
static uint64_t g_diag_next_us;
static unsigned g_timer_calls, g_pump_calls;

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

static void present(void) {
    if (g_off & OFF_PRESENT) return;
    int vw = g_view_320x200 ? GAME_W : sys_video_w();
    int vh = g_view_320x200 ? GAME_H : sys_video_h();
    if (vw <= 0 || vh <= 0) return;

    palette_push();
    uint8_t *fb = of_video_surface();
    const uint8_t *src = g_video_ram;               /* indexed pixels, linear in the A0000 arena */

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

    if (vw == GAME_W && vh == GAME_H) {
        memset(fb, 0, (size_t)BAR_ROWS * PANEL_W);
        uint8_t *dst = fb + (size_t)BAR_ROWS * PANEL_W;
        memcpy(dst, src, (size_t)GAME_W * GAME_H);
        memset(fb + (size_t)(BAR_ROWS + GAME_H) * PANEL_W, 0,
               (size_t)(PANEL_H - BAR_ROWS - GAME_H) * PANEL_W);
    } else if (vw == 2 * PANEL_W && vh == 2 * PANEL_H) {
        for (int y = 0; y < PANEL_H; y++)
            for (int x = 0; x < PANEL_W; x++)
                fb[y * PANEL_W + x] = downscale_2x2(src, x, y);
    } else {
        /* An unexpected mode: copy the overlapping region 1:1 rather than guess a scale. */
        int w = vw < PANEL_W ? vw : PANEL_W;
        int h = vh < PANEL_H ? vh : PANEL_H;
        memset(fb, 0, (size_t)PANEL_W * PANEL_H);
        for (int y = 0; y < h; y++)
            memcpy(fb + (size_t)y * PANEL_W, src + (size_t)y * vw, (size_t)w);
    }
#ifdef OF_PC
    if (getenv("NIGHT_PANEL_SHOT") && (g_frames % (unsigned)(getenv("NIGHT_PANEL_EVERY") ? atoi(getenv("NIGHT_PANEL_EVERY")) : 40)) == 20u)
        panel_shot();
#endif
    of_video_flush();                               /* the display path reads memory the CPU just wrote */
    of_video_flip();
}

/* ---- sound board bring-up (docs/98) ----------------------------------------------------
 * The original only enables its sample player if the device answers: fn_040f4f wants the 0xaa
 * signature, fn_040fb3's probe (0xf2) must raise the device IRQ, fn_0411d9 must read an id.  This is
 * host-side device modelling, identical to src/app/main.c's board_boot(), with no SDL in it. */
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
        present();
        g_frames++;
        g_present_next_us = now + PRESENT_PERIOD_US;
        if (g_frames_limit && g_frames >= (uint32_t)g_frames_limit) g_quit = 1;
    }
    if (now >= g_diag_next_us) {
        g_diag_next_us = now + 1000000ull;
        static uint32_t s_last_ticks;
        printf("memstat frames=%u ticks=%u(+%u/s) pumps=%u arena=%u/%u pit=%u Hz xfer=%u plays=%u cdda=%u/%u/%u\n",
               g_frames, g_timer_calls, g_timer_calls - s_last_ticks, g_pump_calls,
               heap_arena_used(), heap_arena_cap(), sys_pit_hz(0),
               sys_audio_transfers(), g_of_sfx_plays,
               of_cdda_pos(), of_cdda_sectors(), of_cdda_tracks());
        s_last_ticks = g_timer_calls;
    }
    g_depth--;
    if (g_quit) longjmp(g_game_escape, 1);
}


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
    if (getenv("NIGHT_RIG_STEP_CALLS")) {
        int hz = atoi(getenv("NIGHT_RIG_STEP_CALLS"));
        if (hz > 0) g_fixed_step_calls = hz;
    }
    if (getenv("NIGHT_RIG_FPS")) {
        int f = atoi(getenv("NIGHT_RIG_FPS"));
        if (f > 0) g_fixed_fps = f;
    }
    of_video_init();
    of_video_set_color_mode(OF_VIDEO_MODE_8BIT);
    of_audio_init();

    const struct of_capabilities *caps = of_get_caps();
    printf("night pocket: cpu=%u Hz sdram=%u KB heap=%u KB fb=%ux%u stride=%u voices=%u variant=%u\n",
           caps->cpu_freq_hz, caps->sdram_size / 1024, caps->heap_size / 1024,
           caps->fb_width, caps->fb_height, caps->fb_stride, caps->mixer_voices, caps->core_variant);

#ifdef OF_PC
    if (g_rig_dir) rig_screen_init(g_rig_dir, 8);
#endif

    of_settings_load();                        /* os.ini [night] on the device, env on the desktop */
    if (!of_setting_cdda()) g_off |= OFF_CDDA;
    if (!of_setting_pad())  g_off |= OFF_PAD;
    if (g_load_slot >= 0) {                    /* a restore arms the warp basis from the blob */
        if (of_save_read(g_load_slot) != 0 && of_save_last_error()) printf("load: %s\n", of_save_last_error());
    }
    if (g_warp_stage_cli > 0) g_warp_stage = g_warp_stage_cli;
    pad_script_load();
    board_boot();
    audio_boot();
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
    printf("disc: %d tracks, %u sectors\n", tracks, of_disc_total_sectors());

    sys_set_input_pump(input_pump);
    sys_set_tick_pump(tick_pump);
    sys_set_present_hook(present);                 /* docs/325: a pass that writes 0xA0000 and waits asks for one present */

    g_tick_next_us = sys_now_us();
    g_present_next_us = g_tick_next_us;
    g_diag_next_us = g_tick_next_us + 1000000ull;

    g_boot_us = sys_now_us();                       /* the warp-entry pulse timeline (task-7) */
    if (setjmp(g_game_escape) == 0) {
        g_escape_armed = 1;
        dispatch_a(0xff);                          /* 0x422c7: the registered event-table entries */
        fn_0445c9_startup();                       /* 0x0445d8-0445fe: the entry thunk's pre-main half */
        fn_03af18();                               /* THE GAME MAIN */
        printf("the game main returned\n");
    } else {
        printf("game main unwound (quit / fatal)\n");
    }
    g_escape_armed = 0;
    g_quit = 1;
    of_cdda_stop();                 /* closes the witness WAV (and its header fix-up) on the way out */
    present();
    printf("frames presented: %u  ticks: %u  cdda tracks: %u  sfx voices: %u\n",
           g_frames, g_timer_calls, of_cdda_tracks(), g_of_sfx_plays);
    return 0;
}

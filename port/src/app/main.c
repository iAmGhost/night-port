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
#include "f/rig_cells.h"
#include "sys/sys.h"
#include "keymap.h"

/* ---- unaligned flat-model views (docs/126) ----------------------------------
 * The shipped record layout is packed and deliberately unaligned (dword stores at +0x01/+0x05/+0x17 ...
 * read out of the entity constructor fn_010efc, docs/125 S1), so every typed access into it is a misaligned
 * access in C terms.  A packed struct view is alignment-safe on gcc/clang and keeps the exact store width
 * the shipped instruction used; scripts/port/unaligned_casts.py produces the rewrite.
 * The packed attribute is what makes the view safe -- without it the ABI (and UBSan) still require the
 * member's natural alignment, which is exactly what the shipped layout violates. */
#if defined(__GNUC__)
#  define UA_ATTR __attribute__((packed, aligned(1)))
#else
#  define UA_ATTR
#endif
struct UA_ATTR ua32 { uint32_t v; };
struct UA_ATTR ua_uint8tp { uint8_t * v; };

#include <SDL3/SDL.h>

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SCREEN_W 320
#define SCREEN_H 200

static uint8_t   g_node_pool[0x53800];       /* the resource node pool (docs/15 boot recipe) */
static int       g_quit;
static int       g_frame_budget;      /* docs/146 §9: --frames as a budget the game main actually stops at */
static unsigned  g_budget_hits;
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
static int g_resnames;
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

/* ---- presentation ----------------------------------------------------
 * The arena (g_video_ram) is the game's linear screen model; its geometry follows the mode the game
 * set through VBE (fn_03f15f asks for mode 0x101 = 640x480x8), so the texture is (re)made to match. */
static int g_vid_w, g_vid_h;

static int ensure_mode(int w, int h) {
    if (w == g_vid_w && h == g_vid_h && g_tex) return 0;
    if (g_tex) { SDL_DestroyTexture(g_tex); g_tex = NULL; }
    free(g_pixels);
    g_pixels = malloc((size_t)w * (size_t)h * 4u);
    g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!g_tex || !g_pixels) return -1;
    SDL_Log("display mode %dx%d", w, h);
    g_vid_w = w; g_vid_h = h;
    return 0;
}

/* The DAC holds 6-bit values (measured: the game never writes a byte above 63), so scale them the way a
 * VGA DAC does when turning them into 8-bit RGB. */
/* The game keeps its own palette table (DAT_00062d60 / g_pal) and the reference renders in the test
 * harness use it directly, so the host must read it as-is. The old version copied the DAC mirror over
 * g_pal and rescaled it in place, which both washed the picture out and mutated the table the game
 * itself fades through. */
static uint8_t g_palette8[768];
static int  g_pal_src = 0;           /* 0 = the DAC mirror, 1 = the game's own staging table DAT_633b8 */
static void palette_rgba(void) {
    if (g_pal_src == 1) memcpy(g_palette8, g_dat_633b8, 768);
    else if (g_pal_src == 2)
        for (int i = 0; i < 256; i++) {          /* the VBE palette cache DAT_63e50 (colour << shift) */
            uint32_t v = g_dat_63e50[i] >> (g_dat_63e4c & 0x1f);
            g_palette8[i*3] = (uint8_t)v; g_palette8[i*3+1] = (uint8_t)(v >> 8); g_palette8[i*3+2] = (uint8_t)(v >> 16);
        }
    else if (g_pal_src == 3)
        for (int i = 0; i < 256; i++) {
            uint32_t v = g_dat_63e50[i];
            g_palette8[i*3] = (uint8_t)v; g_palette8[i*3+1] = (uint8_t)(v >> 8); g_palette8[i*3+2] = (uint8_t)(v >> 16);
        }
    else                sys_vga_palette(g_palette8);          /* the DAC mirror: what the game last programmed into 0x3c8/0x3c9 */
    if (sys_vga_dac_max() <= 63)                       /* 6-bit DAC values: scale the way a DAC does */
        for (int i = 0; i < 768; i++) g_palette8[i] = (uint8_t)((g_palette8[i] << 2) | (g_palette8[i] >> 4));
}
static long pt_now_ms(void);
static int g_fixed_step_calls = 0;      /* NIGHT_RIG_STEP_CALLS=<n>: the game clock follows the PRESENTED
                                         * FRAME count instead of the wall clock (docs/107). n is the PIT
                                         * rate in Hz the game programs (490); the tick count per frame is
                                         * derived exactly, so a replayed drive produces one tick sequence. */
static long g_fixed_fps = 60;           /* frames per second the fixed-step clock assumes */
void rig_screen_frame(long t_ms, unsigned long long frame, const void *pixels, int w, int h,
                      int ri, int gi, int bi);
void rig_pcm_write(const void *bytes, size_t n);
static const char *g_rig_dir;

/* ---- injected controls (rig-runtime contract: the rig must detect a deliberately broken build) ----
 * NIGHT_RIG_CONTROL selects one fault; the oracles have to notice without being told which. Each one
 * is a *stubbed helper*, not a fake value: the host stops running a piece of the shipped game.
 *   no_timer : the guest's installed timer handler (fn_03ee88) is never called; the pump bookkeeping
 *              stays, so the host still presents frames while game time stands still
 *   no_input : scripted key events are parsed, logged and marked, but never reach the key state
 *   no_draw  : the frame is composed and recorded, never presented */
static int g_rig_ctl;
static void rig_ctl_init(void) {
    const char *rc = getenv("NIGHT_RIG_CONTROL");
    g_rig_ctl = !rc ? 0 : !strcmp(rc, "no_timer") ? 1 : !strcmp(rc, "no_input") ? 2 :
                !strcmp(rc, "no_draw") ? 4 : 0;
    if (g_rig_ctl)
        SDL_Log("rig control injected: %s%s", rc,
                g_rig_ctl == 4 ? " (NOT observable through the port's own screen witness: that "
                                "witness records the composed buffer, before the present decision)" : "");
    else if (rc)
        SDL_Log("rig control ignored: unknown NIGHT_RIG_CONTROL=%s", rc);
}

static void present(void) {
    /* The room path composites a 320x200 frame (pitch 0x140, 0xfa00 bytes) and flips exactly those
     * 64000 bytes to the framebuffer start (fn_038098), while the slide path fills the whole 640x480
     * region (fn_03f462 with count 0x1e0). Present whichever the game last wrote. */
    int vw = g_view_320x200 ? 320 : sys_video_w(), vh = g_view_320x200 ? 200 : sys_video_h();
    if (ensure_mode(vw, vh) != 0) return;
    palette_rgba();
    const uint8_t *src = g_video_ram;                       /* indexed pixels, linear in the arena */
    for (int i = 0; i < vw * vh; i++) {
        uint8_t idx = src[i];
        /* SDL3 SDL_PIXELFORMAT_RGBA8888 masks are R=0xff000000 G=0x00ff0000 B=0x0000ff00
         * A=0x000000ff (verified empirically), so the alpha byte must be the LOW byte. */
        g_pixels[i] = ((uint32_t)g_palette8[idx * 3] << 24) |
                      ((uint32_t)g_palette8[idx * 3 + 1] << 16) |
                      ((uint32_t)g_palette8[idx * 3 + 2] << 8) | 0xffu;
    }
    if (g_rig_dir)
        rig_screen_frame(pt_now_ms(), (unsigned long long)g_frames, g_pixels, vw, vh, 3, 2, 1);
    if (g_rig_ctl & 4) return;                       /* composed and recorded, never shown */
    SDL_UpdateTexture(g_tex, NULL, g_pixels, vw * 4);
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_RenderTexture(g_ren, g_tex, NULL, NULL);
    SDL_RenderPresent(g_ren);
}

/* ---- audio -----------------------------------------------------------------------------
 * The game drives its sound effects the way a Sound Blaster is driven: DSP commands plus a DMA
 * transfer it programs itself. The sys layer recognises that protocol and turns each transfer into
 * PCM bytes; the host pushes them through an SDL audio stream, which also resamples (the board rate
 * is per-sample: DSP command 0x40 + a time constant). */
static SDL_AudioDeviceID g_adev;
static SDL_AudioStream  *g_astream;
static uint32_t          g_astream_rate;
static uint64_t          g_audio_submitted;   /* bytes handed to the SDL stream */
static uint64_t          g_audio_drained;     /* bytes dropped because no device took them */
static char              g_audio_dev[80] = "none";
static int               g_audio_null_sink;
static SDL_AudioSpec     g_audio_dst;      /* what the opened device wants (CD-DA shares it) */

/* One open attempt, logged. The label names the rung of the fallback ladder. */
static SDL_AudioDeviceID try_open(SDL_AudioDeviceID id, const SDL_AudioSpec *dst, const char *label) {
    if (!id) return 0;
    SDL_AudioDeviceID d = SDL_OpenAudioDevice(id, dst);
    if (d) snprintf(g_audio_dev, sizeof g_audio_dev, "%s", label);
    else   SDL_Log("audio: %s refused (%s)", label, SDL_GetError());
    return d;
}

/* The last rung used to be a driver swap: quit the audio subsystem, set SDL_AUDIODRIVER=dummy and
 * re-init. On this SDL3 that sequence hangs after the real devices have all refused (the log stops at
 * the last refusal and the run never ends), so the last rung is instead the software sink in
 * audio_pump: the board's DMA ring is still drained every frame, which is what the game's own
 * busy-wait depends on. An operator can still ask SDL for its null driver up front with
 * SDL_AUDIODRIVER=dummy, which is how the counters below were measured. */
static void audio_pump_drain_only(void);

static void audio_boot(void) {
    if (!SDL_Init(SDL_INIT_AUDIO)) { SDL_Log("audio unavailable: %s", SDL_GetError()); return; }
    SDL_AudioSpec dst; SDL_zero(dst);
    dst.format = SDL_AUDIO_S16; dst.channels = 2; dst.freq = 48000;

    /* Rung 1: the system default. Rung 2: every other output device the host lists (a machine can have
     * a dead default and a working secondary). Rung 3: the null sink. */
    g_adev = try_open(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &dst, "default");
    if (!g_adev) {
        int n = 0;
        SDL_AudioDeviceID *list = SDL_GetAudioPlaybackDevices(&n);
        char label[64];
        for (int i = 0; i < n && !g_adev; i++) {
            const char *nm = SDL_GetAudioDeviceName(list[i]);
            snprintf(label, sizeof label, "alt %u %s", i, nm ? nm : "?");
            g_adev = try_open(list[i], &dst, label);
        }
        SDL_free(list);
    }
    if (!g_adev) { SDL_Log("audio: no stream at all, the board's DMA ring will be drained and discarded"); return; }

    SDL_AudioSpec src; SDL_zero(src);
    src.format = SDL_AUDIO_S16; src.channels = 1; src.freq = 22050;
    g_astream_rate = 22050;
    g_astream = SDL_CreateAudioStream(&src, &dst);
    /* SDL3 only plays a stream once it is bound to an open device. Without this the bytes were
     * accepted by the stream (the counters happily grew) but nothing reached the hardware, which is
     * why the available-byte figure climbed to ~98 KB and stopped there. */
    if (g_astream && !SDL_BindAudioStream(g_adev, g_astream))
        SDL_Log("audio: bind failed: %s", SDL_GetError());
    g_audio_dst = dst;
    SDL_ResumeAudioDevice(g_adev);                            /* SDL3 devices open paused */
    SDL_Log("audio: %s at %d Hz (board samples start at %u Hz)", g_audio_dev, dst.freq, g_astream_rate);
}

/* ---- CD-DA (Red Book) playback ------------------------------------------------------------
 * fn_010b1d (f.c) resolves a track through the disc TOC and asks for its sectors -- on DOS that is
 * the MSCDEX media read behind fn_010570. The seam lands here. Measured on TG_NIGHT.IMG the audio
 * sectors carry **little-endian** 16-bit signed PCM (per-channel lag-1 autocorrelation 0.982 /
 * 0.972, while the big-endian reading of the same bytes is uncorrelated at -0.001 / +0.004), unlike
 * Red Book's on-disc byte order, so the sector bytes go to SDL unchanged: 2352 bytes = 588 stereo
 * frames = exactly 1/75 s. A second stream is bound to the same device, so the board's effects and
 * the disc's music are mixed by SDL rather than by us.
 * With no device at all the track is still consumed on the disc's own 75-sectors-per-second clock,
 * so the counters -- and the game's refresh watchdog, which compares DAT_00063e38/490 against
 * DAT_00053c48 (the track length in seconds) -- behave the same way. */
/* CD-DA witness (app/rig_sink.c): the disc path is where the game's music lives, so the audio oracle
 * needs it as well as the DSP/PCM path. */
void rig_cdda_marker(long t_ms, long index, uint32_t lba, uint32_t sectors);
void rig_cdda_sample(long t_ms, const void *bytes, unsigned n);
static SDL_AudioStream *g_cdda_stream;
static uint32_t  g_cdda_lba, g_cdda_sectors, g_cdda_pos;
static uint64_t  g_cdda_bytes, g_cdda_sector_reads;
static uint32_t  g_cdda_tracks;
static uint64_t  g_cdda_clock, g_cdda_freq;      /* drain-model timing */
static uint8_t   g_cdda_raw[2352];

static void cdda_play(uint32_t index, uint32_t lba, uint32_t sectors) {
    /* MSCDEX semantics: the play request (fn_010570, request 0x1510 behind fn_010b1d's call) replaces
     * whatever was playing -- the drive cannot play two tracks at once. The port reused one SDL stream
     * and never flushed on replacement, so the previous track's queued sectors (the planner keeps a
     * 30-sector / 0.4 s lead) played on top of the new track, and the witness showed NO stop mark at any
     * track change (docs/105 §1: `play 1` at 3.66 s, `play 9` at 8.12 s, no stop for track 1 anywhere).
     * That is the audible half of "music is still broken": every selection overlaps the old track. */
    if (g_cdda_sectors != 0) {
        if (g_rig_dir) rig_cdda_marker(pt_now_ms(), -1, g_cdda_lba, g_cdda_sectors);
        if (g_cdda_stream) SDL_FlushAudioStream(g_cdda_stream);
        g_cdda_sectors = 0; g_cdda_pos = 0;
    }
    g_cdda_tracks++;
    g_cdda_lba = lba; g_cdda_sectors = sectors; g_cdda_pos = 0;
    g_cdda_clock = SDL_GetPerformanceCounter();
    if (!g_cdda_freq) g_cdda_freq = SDL_GetPerformanceFrequency();
    if (g_adev && !g_cdda_stream && g_audio_dst.freq) {
        SDL_AudioSpec src; SDL_zero(src);
        src.format = SDL_AUDIO_S16; src.channels = 2; src.freq = 44100;
        g_cdda_stream = SDL_CreateAudioStream(&src, &g_audio_dst);
        if (g_cdda_stream && !SDL_BindAudioStream(g_adev, g_cdda_stream)) {
            SDL_Log("cdda: bind failed: %s", SDL_GetError());
            SDL_DestroyAudioStream(g_cdda_stream); g_cdda_stream = 0;
        }
    }
    SDL_Log("cdda: track index %u lba=%u sectors=%u (%.1f s)%s", index, lba, sectors,
            sectors / 75.0, g_cdda_stream ? "" : " (no device: counted, not played)");
    if (g_rig_dir) rig_cdda_marker(pt_now_ms(), (long)index, lba, sectors);
}

static void cdda_stop(void) {
    if (g_rig_dir) rig_cdda_marker(pt_now_ms(), -1, g_cdda_lba, g_cdda_sectors);
    g_cdda_sectors = 0; g_cdda_pos = 0;
    if (g_cdda_stream) SDL_FlushAudioStream(g_cdda_stream);
}

static void cdda_pump(void) {
    if (!g_cdda_sectors) return;
    uint32_t want = 0;
    if (g_cdda_stream) {
        want = sys_cd_da_plan(g_cdda_pos, g_cdda_sectors, 30, 16);   /* ~0.4 s lead, 16-sector chunks */
    } else {
        uint64_t now = SDL_GetPerformanceCounter();
        if (!g_cdda_freq) g_cdda_freq = SDL_GetPerformanceFrequency();
        uint64_t el = (now - g_cdda_clock) * 75 / g_cdda_freq;       /* sectors of wall time */
        g_cdda_clock += el * g_cdda_freq / 75;
        want = sys_cd_da_plan(g_cdda_pos, g_cdda_sectors, (uint32_t)(el ? el : 1), 16);
    }
    for (uint32_t i = 0; i < want; i++) {
        if (cd_iso_read_audio(g_cdda_lba + g_cdda_pos, g_cdda_raw) != 0) { g_cdda_sectors = 0; break; }
        if (g_cdda_stream && !SDL_PutAudioStreamData(g_cdda_stream, g_cdda_raw, 2352)) { g_cdda_sectors = 0; break; }
        g_cdda_bytes += 2352; g_cdda_sector_reads++; g_cdda_pos++;
        if (g_rig_dir) rig_cdda_sample(pt_now_ms(), g_cdda_raw, 2352);
    }
}

/* No host stream: drain the board's ring into a scratch buffer. The game waits on the *board* (its DMA
 * transfer completing and the DSP accepting a stop), not on the host device, so the emulated DMA clock
 * has to keep advancing or fn_04185e never returns and the game logic starves. */
static void audio_pump_drain_only(void) {
    static int16_t sink[2048];
    sys_audio_set_source(g_dat_63fb4);
    uint32_t got;
    while ((got = sys_audio_fill(sink, 2048)) != 0) {
        /* No host device is a headless rig, not a mute rig: the board's bytes are still produced, and
         * the PCM witness still has to exist (docs/34: the game waits on the board, not the device). */
        if (g_rig_dir) rig_pcm_write(sink, (size_t)got * 2);
        g_audio_drained += got * 2;
    }
    if (getenv("NIGHT_AUDIODBG") && (g_frames % 60) == 0)
        SDL_Log("audiodbg: frame=%u no-stream drained=%llu B transfers=%u stops=%u rate=%u width=%u "
                "pending=%u pushed=%u", g_frames, (unsigned long long)g_audio_drained,
                sys_audio_transfers(), sys_audio_stops(), sys_audio_rate(), sys_audio_width(),
                sys_audio_pending(), sys_audio_pushed());
}

/* The game's sound board is only initialised if the hardware answers: fn_040f4f scans the device
 * ports for the 0xaa signature (after fn_04123d's reset pulse on base+6, the classic SB16 detection),
 * fn_040fb3 needs its probe command 0xf2 (outb base+0xc) to raise the device interrupt, and fn_0411d9
 * needs a non-zero id (command 0xE1, two data reads). Without that DAT_00063f90 stays 0 and fn_04185e
 * (the sample player) returns at its first gate -- every in-game SFX call is swallowed (docs/98).
 * docs/98 measured the ORIGINAL under dosbox-automation (sbtype=sb16): at the title DAT_53334=0x220,
 * DAT_63f84=7 (the probe's IRQ fired vector 0x0f), DAT_63f88=0x405 (id 4.5 = SB16), DAT_63f90=1,
 * DAT_51a10=1. The model below reproduces exactly those values; NIGHT_BOARD=0 opts out. */
static void board_cmd_hook(uint16_t port, uint8_t cmd) {
    if (port != 0x22c) return;                      /* the command/write port base+0xc (docs/98 0x41375) */
    if (cmd == 0xf2) sys_isr_fire(0x0f);            /* IRQ7: the probe measures DAT_63f84=7 (docs/98) */
    if (cmd == 0xe1) {
        static const uint8_t id[] = { 0x04, 0x05 }; /* fn_0411d9: major, minor -> DAT_63f88=0x405 */
        sys_dev_read_seq(0x22a, id, 2);
    }
}
static void board_boot(void) {
    static const uint8_t sig[] = { 0xaa };
    sys_dev_read_seq(0x22a, sig, 1);                  /* the detection probe's signature read */
    sys_dev_status_set(0x22e, 0x80);                  /* read-ready status (fn_0412e9 wants bit7 set) */
    sys_dev_status_set(0x22c, 0x00);                  /* write-ready status (fn_041390 wants bit7 clear) */
    sys_isr_bind(0x41439, fn_041439);                 /* the game's own vector-0x0a/0x0b/0x0d/0x0f/0x72 */
    sys_isr_bind(0x41486, fn_041486);                 /* probe stubs (docs/98); whichever fires names */
    sys_isr_bind(0x414d3, fn_0414d3);                 /* its IRQ in DAT_63f84 */
    sys_isr_bind(0x41520, fn_041520);
    sys_isr_bind(0x4156d, fn_04156d);
    sys_dev_cmd_hook(board_cmd_hook);
    g_dat_53334 = 0x220;      /* the value the original runs with at the title (docs/98 live read) */
}

static void audio_pump(void) {
    /* NIGHT_PLAYVOC=<1..100>: hand one shipped sound slot to the game's own sample player once, so the
     * .voc -> command-stream -> DSP/DMA -> SDL path is measurable with real assets. The in-game triggers
     * for these slots are action-record opcodes (SND -> fn_0113e7, plus fn_03f6bc), which a blind key
     * script does not reach, so this is the deterministic way to prove the backend works end to end. */
    static int voc_done;
    if (!voc_done && g_frames > 300 && getenv("NIGHT_PLAYVOC")) {
        int n = atoi(getenv("NIGHT_PLAYVOC"));
        if (n >= 1 && n <= 100 && g_dat_53d0c[n - 1]) {
            voc_done = 1;
            SDL_Log("playvoc: slot %d cursor=%p first bytes=%02x %02x %02x %02x",
                    n, (void *)g_dat_53d0c[n - 1], g_dat_53d0c[n-1][0], g_dat_53d0c[n-1][1],
                    g_dat_53d0c[n-1][2], g_dat_53d0c[n-1][3]);
            fn_04185e(g_dat_53d0c[n - 1]);
        }
    }
    if (!g_astream) { audio_pump_drain_only(); return; }
    sys_audio_set_source(g_dat_63fb4);                 /* the buffer an armed DMA transfer reads */
    uint32_t rate = sys_audio_rate();
    if (rate != g_astream_rate && rate) {              /* DSP 0x40 changed the board rate */
        SDL_AudioSpec src; SDL_zero(src);
        src.format = SDL_AUDIO_S16; src.channels = 1; src.freq = (int)rate;
        if (SDL_SetAudioStreamFormat(g_astream, &src, NULL)) g_astream_rate = rate;
    }
    static int16_t buf[2048];
    uint32_t got;
    while ((got = sys_audio_fill(buf, 2048)) != 0) {
        if (g_rig_dir) {
            /* Rig mode: the port half of the audio oracle must not depend on a host device consuming.
             * With SDL_AUDIODRIVER=dummy the stream is never drained, so the latency bound below used to
             * break out of this loop after ~2 s of a 300 s route and pcm.wav came out silent (docs/129). */
            rig_pcm_write(buf, (size_t)got * 2);
            g_audio_submitted += got * 2;
            continue;
        }
        if (SDL_GetAudioStreamAvailable(g_astream) > 96000) break;   /* bound the latency */
        if (!SDL_PutAudioStreamData(g_astream, buf, (int)(got * 2))) { g_audio_drained += got * 2; break; }
        g_audio_submitted += got * 2;
    }
    if (getenv("NIGHT_AUDIODBG") && (g_frames % 60) == 0)
        SDL_Log("audiodbg: frame=%u dev=%s submitted=%llu B drained=%llu B stream=%d B "
                "transfers=%u stops=%u rate=%u width=%u pending=%u pushed=%u",
                g_frames, g_audio_dev, (unsigned long long)g_audio_submitted,
                (unsigned long long)g_audio_drained, SDL_GetAudioStreamAvailable(g_astream),
                sys_audio_transfers(), sys_audio_stops(), sys_audio_rate(), sys_audio_width(),
                sys_audio_pending(), sys_audio_pushed());
}

static int      g_autokey;                   /* headless proof: press confirm after the attract show */
static Uint64   g_start_time;
static int      g_autokey_last;
/* Real-time scripted key press: a key wait (fn_03a224's drains, the room-loop key waits) only runs the
 * input pump, so the timeline has to be clocked off the clock, not off presented frames. */
static void autokey(void) {
    if (!g_autokey) return;
    Uint64 now = SDL_GetPerformanceCounter();
    double t = (double)(now - g_start_time) / (double)g_perf_freq;
    /* Repeating confirm pulse: the attract chain (logo hold, opening .fli, title wipe + drain, then the
     * menu itself) each waits for a key, so a single press is consumed by whichever phase is up. One
     * pulse every 5 s eventually lands while the menu proper is reading it, which is what starts the
     * game (selection 0 -> fn_03af18 leaves the menu loop for the stage). */
    int on = (t > 3.0) && (fmod(t, 5.0) < 0.15);
    if (on != g_autokey_last) {
        g_dat_63ea4 = (uint8_t)on; g_dat_62c58[0x1c] = (uint8_t)on;
        g_autokey_last = on;
        SDL_Log("autokey: confirm %s (t=%.1fs)", on ? "down" : "up", t);
    }
    /* Once the stage is up, walk and swing: the element/AI sound triggers (fn_0113e7, fn_03f6bc and the
     * action-chain sound ids) only fire from real in-game events, so a headless proof of the sample path
     * needs gameplay, not just a menu key. The key-state array is the same surface fn_034635 maintains,
     * and 4 is the state its state machine reads as "held". */
    if (t > 12.0) {
        static const uint8_t keys[] = { 0x4b, 0x4d, 0x48, 0x50, 0x1c, 0x39 };
        static int prev_key = -1;
        int slot = (int)(t * 2) % 6;                    /* one key per 0.5 s */
        int held = (fmod(t, 0.5) < 0.3);
        if (slot != prev_key) { g_dat_62c58[prev_key >= 0 ? prev_key : 0x4b] = 0; prev_key = slot; }
        g_dat_62c58[keys[slot]] = (uint8_t)(held ? 4 : 0);
    }
}

/* ---- the playtest harness: scripted keys + named-event screenshots (docs/46) -----------
 * A script file pins the timeline a human would use (attract -> menu -> episode slide -> stage ->
 * menu screens), and every screenshot is named after the game-visible event that produced it, so the
 * captures taken from the original under DOSBox (scripts/orig_capture.sh) can be paired with them.
 * Script syntax (one directive per line, '#'-comments allowed):
 *     <t_ms> key <name> down|up      press/release a key at t_ms
 *     <t_ms> shot <name>            take a named screenshot at t_ms
 * Key names: enter space esc tab up down left right ctrl shift, single letters/digits, or a hex
 * DOS scan code (0x4b / 4b). */
/* PT_WAIT anchors the rest of the timeline to a game-visible event instead of the wall clock: the
 * script pauses until the resource lookup named in `name` happens (g_res_trace / g_res_trace_seq are the
 * host-side instrumentation for that), then rebases every later event by the slippage. That is what makes
 * a scripted run land the same keys in the same game states regardless of host speed -- with an audio
 * device live the game runs at a different rate and a purely wall-clock script fires the menu keys in the
 * wrong places (docs/55). The leading number on a wait line is a timeout in ms, so a missing anchor cannot
 * hang the run. */
struct pt_evt { long t_ms; int kind; uint8_t sc; int make; char name[40]; uint32_t seq_at, a2, a3; };
static void shot_named(const char *event);
static void key_apply(uint8_t sc, int make);
enum { PT_KEY = 0, PT_SHOT = 1, PT_WAIT = 2, PT_WAIT_DS = 3, PT_POKE_DS = 4, PT_ENT_POKE = 5, PT_ACT_DUMP = 6 };
static struct pt_evt *g_pt;
static int g_pt_n, g_pt_i;
static const char *g_shots_dir;

/* rig_sink (app/rig_sink.c): state-oracle dump; the table itself is generated from the shipped
 * symbol database by scripts/audit/gen_headers.py. */
void rig_init(const char *dir, int hz);
void rig_poll(long t_ms, unsigned long long frame, unsigned long long audio_bytes);
void rig_marker(long t_ms, const char *fmt, ...);
int  rig_ds_read(uint32_t ds, uint32_t *out);   /* current value of a DS-claiming cell */
int  rig_ds_write(uint32_t ds, uint32_t v);     /* scripted injection into the same table */
int  rig_ent_poke(int slot, uint32_t off, uint32_t v);   /* write record+off in the +0x13f list walk */
uint8_t *rig_ent_slot(int slot);                /* the dumper's entity walk, by slot index */
int  rig_player_poke(uint32_t off, uint32_t v);  /* write record+off of the player entity */
int  rig_dump_act(int type_idx, unsigned n, const char *dir);  /* raw action-container capture */
void rig_rebase(long long slip_ms);            /* move the dump timeline onto the script's anchor */
void rig_close(void);
int  rig_census_begin(void);                    /* per-function entry census (docs/120) */
void rig_census_set_path(const char *path);
unsigned rig_census_report(const char *path);
void rig_screen_init(const char *dir, int witness_every);
void rig_pcm_open(const char *dir);
static const char *g_rig_dir;
static int g_rig_hz = 10;
static FILE *g_manifest;
static int g_shot_seq;
static int g_shot_limit = 600;            /* NIGHT_SHOTLIMIT: a dense 20-55 Hz series across a 90 s run needs
                                          * thousands of frames, and the 600 cap silently truncated the series right
                                          * where the stage transition should have been */
static int g_shot_scene = 1, g_shot_res = 1, g_shot_fli = 1;
/* Opening a host audio device can block outright on a machine whose CoreAudio devices are broken (the
 * open call never returns, not merely fails), which must not stop a scripted playtest run. */
static int g_no_audio;
static uint32_t g_pt_res_seq;
static Uint64 g_pt_last_scene, g_pt_last_fli;
static uint64_t g_pt_hash;
static uint8_t g_pt_sample[2048];

static long pt_now_ms(void) {
    /* Fixed step (docs/107): the script clock, the rig stamps and the PIT handler all follow the
     * presented-frame counter, so two runs of one drive are the same game. Wall-clock pumping made
     * back-to-back runs diverge (26 vs 67 resolved hits on ep1_combat with the LCG seed pinned),
     * because a host that falls behind re-anchors and drops ticks: keys then landed on other ticks. */
    if (g_fixed_step_calls) return (long)((unsigned long long)g_frames * 1000ull / (unsigned long long)g_fixed_fps);
    return (long)((SDL_GetPerformanceCounter() - g_start_time) * 1000ull / g_perf_freq);
}

/* the DOS scan code the game's key state consults for a script token */
static uint8_t pt_sc(const char *name) {
    static const struct { const char *n; uint8_t sc; } tbl[] = {
        { "enter", 0x1c }, { "return", 0x1c }, { "space", 0x39 }, { "esc", 0x01 },
        { "tab", 0x0f }, { "up", 0x48 }, { "down", 0x50 }, { "left", 0x4b }, { "right", 0x4d },
        { "ctrl", 0x1d }, { "lctrl", 0x1d }, { "rctrl", 0x1d },
        { "shift", 0x2a }, { "lshift", 0x2a }, { "rshift", 0x36 },
        { "alt", 0x38 }, { "lalt", 0x38 }, { "ralt", 0x38 }, { "altgr", 0x38 },
        { "backspace", 0x0e },
    };
    for (unsigned k = 0; k < sizeof tbl / sizeof tbl[0]; k++)
        if (!strcasecmp(name, tbl[k].n)) return tbl[k].sc;
    if (name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) return (uint8_t)strtoul(name, NULL, 16);
    if (strlen(name) == 2 && isxdigit((unsigned char)name[0]) && isxdigit((unsigned char)name[1]))
        return (uint8_t)strtoul(name, NULL, 16);
    SDL_Scancode c = SDL_GetScancodeFromName(name);
    /* DOS set-1 make codes for A-Z. NOT linear: SDL scancodes are HID usage order (A=4..Z=29), so the
     * old arithmetic fallback mapped letters wrong ("z" became 0x37 instead of 0x2c -- docs/92). */
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const uint8_t dos[] = {
        0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,
        0x32,0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c };
    if (c >= SDL_SCANCODE_A && c <= SDL_SCANCODE_Z)
        return dos[c - SDL_SCANCODE_A];
    if (c >= SDL_SCANCODE_1 && c <= SDL_SCANCODE_0) return (uint8_t)(0x02 + (c - SDL_SCANCODE_1));
    return 0;
}

static void pt_load(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { SDL_Log("script: cannot open %s", path); return; }
    g_pt = malloc(sizeof *g_pt * 512);
    char line[256];
    while (fgets(line, sizeof line, fp) && g_pt_n < 512) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        long t; char verb[64], arg[64], updown[64], argv4[64];
        int n = sscanf(line, "%ld %63s %63s %63s %63s", &t, verb, arg, updown, argv4);
        if (n < 2) continue;
        struct pt_evt *e = &g_pt[g_pt_n];
        e->t_ms = t; e->kind = PT_KEY; e->make = 1; e->name[0] = 0; e->sc = 0; e->seq_at = 0;
        if (!strcasecmp(verb, "shot")) {
            e->kind = PT_SHOT; snprintf(e->name, sizeof e->name, "%s", arg);
        } else if (!strcasecmp(verb, "wait") && n >= 3 && !strcasecmp(arg, "res")) {
            e->kind = PT_WAIT; snprintf(e->name, sizeof e->name, "%s", updown);
            e->seq_at = 0;   /* set when the wait becomes current, see pt_drive */
            if (e->name[0] == 0) { SDL_Log("script: wait res needs a name (line ignored)"); continue; }
        } else if (!strcasecmp(verb, "wait") && n >= 4 && !strcasecmp(arg, "ds")) {
            /* `t wait ds 0x63354 !=0` anchors the rest of the timeline on a shipped DS event. The
             * original side reads the same address through the emulator's memory API, so both sides sit
             * in the same game state when the following keys fire. */
            e->kind = PT_WAIT_DS;
            e->seq_at = (uint32_t)strtoul(updown, NULL, 0);
            snprintf(e->name, sizeof e->name, "%s", argv4);
            if (!e->seq_at) { SDL_Log("script: wait ds needs an address (line ignored)"); continue; }
        } else if (!strcasecmp(verb, "poke") && n >= 3) {
            /* State injection (docs/118): `t poke 0x639bc 0` writes a DS global through the rig's own
             * cell table. Keypress-only replay cannot reach rare states (a knockdown needs a second hit
             * while the victim is still reeling, docs/117), so forcing the precondition makes the shipped
             * transition reachable on demand and comparable on both sides. */
            e->kind = PT_POKE_DS;
            e->seq_at = (uint32_t)strtoul(arg, NULL, 0);
            e->a2 = (uint32_t)strtoul(updown, NULL, 0);
            if (!e->seq_at) { SDL_Log("script: poke needs a DS address (line ignored)"); continue; }
        } else if (!strcasecmp(verb, "heapdump")) {
            /* `t heapdump 0x400000` captures the whole guest heap for offline structure decoding. */
            e->kind = PT_ACT_DUMP;
            e->seq_at = 0xffffffffu;                          /* sentinel: whole heap */
            e->a2 = (n >= 3) ? (uint32_t)strtoul(arg, NULL, 0) : 0x400000u;
        } else if (!strcasecmp(verb, "actdump") && n >= 3) {
            /* `t actdump 31` writes the loaded action container of entity type 31 next to the rig dump. */
            e->kind = PT_ACT_DUMP;
            e->seq_at = (uint32_t)strtoul(arg, NULL, 0);
            e->a2 = (n >= 4) ? (uint32_t)strtoul(updown, NULL, 0) : 0x800u;
        } else if (!strcasecmp(verb, "play") && n >= 4) {
            /* `t play 0x17 0x28` writes record+off of THE PLAYER entity. The shipped hit code selects the
             * reaction type through the striker's state (docs/118 §2), so combo-position probes address the
             * player directly instead of guessing its slot in the list walk. */
            e->kind = PT_ENT_POKE;
            e->seq_at = 0xfffffffeu;                         /* sentinel: the player entity */
            e->a2 = (uint32_t)strtoul(arg, NULL, 0);         /* offset in the record */
            e->a3 = (uint32_t)strtoul(updown, NULL, 0);      /* value */
        } else if (!strcasecmp(verb, "ent") && n >= 4) {
            /* `t ent 2 0x17 0x15` writes 4 bytes at record+0x17 of entity slot 2 in the same +0x13f list
             * walk the state dumper uses, so the injected record is the one the oracle also samples. */
            e->kind = PT_ENT_POKE;
            e->seq_at = (uint32_t)strtoul(arg, NULL, 0);      /* slot */
            e->a2 = (uint32_t)strtoul(updown, NULL, 0);       /* offset in the record */
            e->a3 = (uint32_t)strtoul(argv4, NULL, 0);        /* value */
        } else {
            const char *kn = (!strcasecmp(verb, "key") && n >= 3) ? arg : verb;
            if (!strcasecmp(verb, "key") && n >= 3) e->make = !strcasecmp(updown, "down") || !strcasecmp(updown, "1");
            else if (n >= 3) e->make = !strcasecmp(arg, "down") || !strcasecmp(arg, "1");
            e->sc = pt_sc(kn);
            if (!e->sc) { SDL_Log("script: unknown key '%s' (line ignored)", kn); continue; }
        }
        g_pt_n++;
    }
    fclose(fp);
    for (int i = 1; i < g_pt_n; i++)                    /* stable insertion sort: scripts are almost sorted */
        for (int j = i; j > 0 && g_pt[j].t_ms < g_pt[j - 1].t_ms; j--) {
            struct pt_evt tmp = g_pt[j]; g_pt[j] = g_pt[j - 1]; g_pt[j - 1] = tmp;
        }
    SDL_Log("script: %d events from %s", g_pt_n, path);
}

/* press/release, through the same bytes sdl_events feeds the game */
/* Every lookup the game performs, recorded as it happens (sys_res_notify), so a `wait res` line can match
 * a lookup even when the script polls long afterwards. */
static void shot_queue(const char *event);   /* defined below; queued (delayed) screenshot per event name */
static void shot_queue_scene(const char *event);   /* ditto, but anchored on the next scene change */

static char g_reslog[256][24];
static int  g_reslog_n, g_reslog_cursor;
static char g_res_match[24];
static long g_pt_done_ms = -1;                       /* when the last scripted event fired, -1 = not yet */
static int g_reslog_verbose = -1;          /* NIGHT_RESLOG=1: print every lookup in order (feature-sweep
                                           * inventory: which .spr/.act/.pcx families a given episode touches) */
static void reslog_add(const char *name) {
    if (g_rig_dir) rig_marker(pt_now_ms(), "res %s", name);   /* event anchor for the rig (docs/56) */
    if (g_reslog_verbose < 0) { const char *e = getenv("NIGHT_RESLOG"); g_reslog_verbose = (e && e[0] == '1'); }
    if (g_reslog_verbose) SDL_Log("resseq %-22s n=%d", name, g_reslog_n);
    if (g_reslog_n < (int)(sizeof g_reslog / sizeof g_reslog[0])) {
        snprintf(g_reslog[g_reslog_n], sizeof g_reslog[0], "%s", name);
        g_reslog_n++;
    }
    /* docs/61: a nested screen (fn_038f3d, the level-up picture) announces every asset it loads --
     * levelup2.pcx, levelup.spr, number1.spr, number2.spr -- before its own frame loop reaches a spin
     * point, so the sequence-number edge in shots_poll only ever sees the LAST name of that burst. Queuing a
     * delayed hold shot per announce names each asset with a frame of the screen it belongs to (the delay
     * lets the screen composite first), which is what makes those screens auditable by name. */
    shot_queue_scene(name);
  }

static void pt_drive(void) {
    if (!g_pt || !g_pt_n) return;
    long t = pt_now_ms();
    while (g_pt_i < g_pt_n) {
        struct pt_evt *e = &g_pt[g_pt_i];
        if (e->kind == PT_ACT_DUMP) {
            int w = rig_dump_act(e->seq_at == 0xffffffffu ? -1 : (int)e->seq_at, e->a2, g_rig_dir);
            SDL_Log("script: actdump type %u -> %d bytes at t=%ldms", e->seq_at, w, t);
            g_pt_i++;
            continue;
        }
        if (e->kind == PT_POKE_DS) {
            int w = rig_ds_write(e->seq_at, e->a2);
            SDL_Log("script: poke ds %#x = %u (%s) at t=%ldms", e->seq_at, e->a2,
                    w ? "ok" : "no such cell", t);
            if (g_rig_dir) rig_marker(t, "poke ds %#x = %u", e->seq_at, e->a2);
            g_pt_i++;
            continue;
        }
        if (e->kind == PT_ENT_POKE) {
            int w = (e->seq_at == 0xfffffffeu) ? rig_player_poke(e->a2, e->a3)
                                               : rig_ent_poke((int)e->seq_at, e->a2, e->a3);
            SDL_Log("script: %s +%#x = %#x (%s) at t=%ldms",
                    e->seq_at == 0xfffffffeu ? "poke player" : "poke ent", e->a2, e->a3,
                    w ? "ok" : "no such object", t);
            if (g_rig_dir) rig_marker(t, "poke %s +%#x = %#x",
                                      e->seq_at == 0xfffffffeu ? "player" : "ent", e->a2, e->a3);
            g_pt_i++;
            continue;
        }
        if (e->kind == PT_WAIT_DS) {
            uint32_t v = 0;
            int sz = rig_ds_read(e->seq_at, &v);
            const char *num = e->name + 2;              /* "==N" "!=N" ">=N" "<=N" */
            uint32_t want = (uint32_t)strtoul(num, NULL, 0);
            int hit = sz && (!strcmp(e->name, "==") ? v == want
                             : e->name[0] == '!' ? v != want
                             : e->name[0] == '>' ? v >= want : v <= want);
            if (hit) {
                long slip = t - e->t_ms;
                for (int k = g_pt_i + 1; k < g_pt_n; k++) g_pt[k].t_ms += slip;
                if (g_rig_dir) rig_rebase(slip);
                g_pt_i++;
                SDL_Log("script: anchored on ds %#x = %u at t=%ldms (rebased %+ldms)",
                        e->seq_at, v, t, slip);
                if (g_rig_dir) rig_marker(t, "ds %#x = %u", e->seq_at, v);
                continue;
            }
            if (t >= e->t_ms) {
                SDL_Log("script: wait ds %#x %s timed out at t=%ldms val=%u (continuing)",
                        e->seq_at, e->name, t, v);
                g_pt_i++;
                continue;
            }
            return;
        }
        if (e->kind == PT_WAIT) {
            /* A wait is polled every pass, not scheduled: its number is only the deadline. The anchor
             * lookup can happen long before that number, and the game's own resource lookups keep moving
             * g_res_trace, so the match has to be against "some lookup since the wait became current". */
            if (!e->seq_at) e->seq_at = g_res_trace_seq;
            int hit_at = -1;
            for (int k = g_reslog_cursor; k < g_reslog_n; k++)
                if (strcasestr(g_reslog[k], e->name)) { hit_at = k; break; }
            if (hit_at >= 0) {
                g_reslog_cursor = hit_at + 1;
                snprintf(g_res_match, sizeof g_res_match, "%s", g_reslog[hit_at]);
                long slip = t - e->t_ms;                     /* the rest of the timeline is relative to the anchor */
                for (int k = g_pt_i + 1; k < g_pt_n; k++) g_pt[k].t_ms += slip;
                if (g_rig_dir) rig_rebase(slip);
                g_pt_i++;
                SDL_Log("script: anchored on res '%s' at t=%ldms (rebased %+ldms)", g_res_match, t, slip);
                continue;
            }
            if (t >= e->t_ms) {
                SDL_Log("script: wait res '%s' timed out at t=%ldms (continuing)", e->name, t);
                g_pt_i++;
                continue;
            }
            return;                                          /* still waiting: nothing later may fire */
        }
        if (e->t_ms > t) return;
        g_pt_i++;
        if (g_pt_i >= g_pt_n) g_pt_done_ms = t;      /* script exhausted: let the host wrap up cleanly */
        if (e->kind == PT_SHOT) { shot_named(e->name); continue; }
        SDL_Log("script: key %#02x %s (t=%ldms)", e->sc, e->make ? "down" : "up", e->t_ms);
        if (g_rig_dir) rig_marker(e->t_ms, "key %#02x %s", e->sc, e->make ? "down" : "up");
        if (g_rig_ctl & 2)
            SDL_Log("rig control: key %#02x %s withheld", e->sc, e->make ? "down" : "up");
        else
            key_apply(e->sc, e->make);
    }
}

/* ---- named-event screenshots ------------------------------------------------------------- */
static void sanitize(char *s) {
    for (; *s; s++) if (!isalnum((unsigned char)*s)) *s = (*s == '.' || *s == '-' ) ? *s : '_';
}

static void shot_named(const char *event) {
    if (!g_shots_dir || g_shot_seq >= g_shot_limit) return;
    SDL_CreateDirectory(g_shots_dir);
    int W = g_vid_w ? g_vid_w : sys_video_w(), H = g_vid_h ? g_vid_h : sys_video_h();
    if (W <= 0 || H <= 0) return;
    palette_rgba();
    char safe[64]; snprintf(safe, sizeof safe, "%s", event && *event ? event : "shot"); sanitize(safe);
    char path[600];
    snprintf(path, sizeof path, "%s/%04d_%s.bmp", g_shots_dir, g_shot_seq, safe);
    SDL_Surface *s = SDL_CreateSurface(W, H, SDL_PIXELFORMAT_RGBA8888);
    if (!s) return;
    uint32_t *px = (uint32_t *)s->pixels;
    for (int i = 0; i < W * H; i++) {
        uint8_t idx = g_video_ram[i];
        px[i] = ((uint32_t)g_palette8[idx * 3] << 24) | ((uint32_t)g_palette8[idx * 3 + 1] << 16) |
                ((uint32_t)g_palette8[idx * 3 + 2] << 8) | 0xffu;
    }
    int ok = SDL_SaveBMP(s, path);
    SDL_DestroySurface(s);
    if (getenv("NIGHT_VIDRAW")) {
        /* Diagnostic dump of the *arena bytes* behind a shot, not the composed bitmap: it separates a
         * game-side write problem (the RLE loader put the wrong bytes in the screen model) from a host-side
         * presentation problem (the model is right but the palette/geometry applied to it is not).  If the
         * game's screen base is not the arena base, that copy follows a marker. */
        char rp[700]; snprintf(rp, sizeof rp, "%s/%04d_%s.raw", g_shots_dir, g_shot_seq, safe);
        FILE *fp = fopen(rp, "wb");
        if (fp) {
            fwrite(g_video_ram, 1, (size_t)W * H, fp);
            ptrdiff_t delta = (ptrdiff_t)((const uint8_t *)g_dat_633b0 - g_video_ram);
            fprintf(fp, "#base_delta=%td\n#pal", delta);
            fwrite(g_palette8, 1, 768, fp);        /* the DAC as the host sees it right now */
            fclose(fp);
        }
    }
    if (!g_manifest) {
        char mp[600]; snprintf(mp, sizeof mp, "%s/manifest.tsv", g_shots_dir);
        g_manifest = fopen(mp, "wb");
        if (g_manifest) fputs("seq\tt_ms\tframes\tevent\tresource\tstage\tentities\tW\tH\n", g_manifest);
    }
    if (g_manifest)
        fprintf(g_manifest, "%d\t%ld\t%u\t%s\t%s\t%u\t%d\t%d\t%d\n", g_shot_seq, pt_now_ms(), g_frames,
                safe, g_res_trace, (unsigned)g_dat_531f4, (int)g_dat_53c68, W, H);
    SDL_Log("shot %04d %-18s frame=%u t=%ldms res=%s stage=%u entities=%d %s",
            g_shot_seq, safe, g_frames, pt_now_ms(), g_res_trace, (unsigned)g_dat_531f4,
            (int)g_dat_53c68, ok ? "" : "(save failed)");
    g_shot_seq++;
}

/* Named events fire the moment a resource is OPENED, which is usually before the fade-in has put
 * anything on screen -- a `logo.pcx` shot taken at lookup time is a black frame and proves nothing. So
 * every named event also queues a "hold" shot a fixed time later, once the retrace-paced fades have
 * brought the picture up (docs/46). --shot-delay <ms> tunes it (0 disables). */
#define PT_MAX_PENDING 12
/* `due` is a wall-clock fallback; `scene_gen0 >= 0` makes the shot wait for the NEXT scene change instead
 * (see shot_queue_scene), which is what an asset announced by a nested screen needs: fn_038f3d loads
 * levelup2.pcx / levelup.spr / number1.spr / number2.spr at screen entry, but the picture is only composited
 * ~700 frames later, so a plain time-delayed hold photographed the previous screen (docs/61). */
static struct { char name[72]; Uint64 due; long scene_gen0; } g_pt_pending[PT_MAX_PENDING];
static long g_scene_gen = 0;            /* bumped on every detected scene change */
static int    g_pt_pending_n;
static Uint64 g_shot_delay_ms = 1000;   /* fn_040660 / fn_0406f5 need ~0.7 s of retraces to reach full brightness */

static void shot_queue(const char *event) {
    if (!g_shot_delay_ms) return;
    char nm[72]; snprintf(nm, sizeof nm, "%s_hold", event);
    for (int i = 0; i < g_pt_pending_n; i++)
        if (strcmp(g_pt_pending[i].name, nm) == 0) return;      /* one hold shot per event name */
    if (g_pt_pending_n >= PT_MAX_PENDING) return;
    snprintf(g_pt_pending[g_pt_pending_n].name, sizeof g_pt_pending[0].name, "%s", nm);
    g_pt_pending[g_pt_pending_n].due = SDL_GetPerformanceCounter() +
                                       (Uint64)g_shot_delay_ms * g_perf_freq / 1000;
    g_pt_pending[g_pt_pending_n].scene_gen0 = -1;              /* time-anchored */
    g_pt_pending_n++;
}

/* Queue a shot for the next scene change (with a wall-clock fallback), used for assets announced by a
 * nested screen: the load happens at screen entry, the picture appears seconds later. */
static void shot_queue_scene(const char *event) {
    if (!g_shots_dir) return;
    char nm[72]; snprintf(nm, sizeof nm, "%s_hold", event);
    for (int i = 0; i < g_pt_pending_n; i++)
        if (strcmp(g_pt_pending[i].name, nm) == 0) return;
    if (g_pt_pending_n >= PT_MAX_PENDING) return;
    snprintf(g_pt_pending[g_pt_pending_n].name, sizeof g_pt_pending[0].name, "%s", nm);
    g_pt_pending[g_pt_pending_n].due = SDL_GetPerformanceCounter() +
                                       (Uint64)(g_shot_delay_ms ? g_shot_delay_ms : 1000) * 6 * g_perf_freq / 1000;
    g_pt_pending[g_pt_pending_n].scene_gen0 = g_scene_gen;
    g_pt_pending_n++;
}

static void shot_flush_pending(void) {
    Uint64 now = SDL_GetPerformanceCounter();
    for (int i = 0; i < g_pt_pending_n; i++) {
        int scene_ready = g_pt_pending[i].scene_gen0 >= 0 && g_scene_gen > g_pt_pending[i].scene_gen0;
        if (!scene_ready && now < g_pt_pending[i].due) continue;
        char nm[72]; snprintf(nm, sizeof nm, "%s", g_pt_pending[i].name);
        g_pt_pending[i] = g_pt_pending[--g_pt_pending_n];       /* drop before the shot can recurse */
        shot_named(nm);
        i = -1;                                                /* the array shifted; restart */
    }
}

/* Scene/resource watcher: a resource lookup is an event with a name, a large screen change without one
 * is a scene boundary, and a .fli playback gets a periodic shot so pacing is measurable. */
/* The resource edge, factored out so nested screens are visible too. A whole screen such as the level-up
 * picture (fn_038f3d) runs inside ONE host step -- it does not return to the frame pump until the screen
 * exits -- so its own resource lookups (levelup2.pcx, levelup.spr, number1/2.spr) were announced but never
 * got a frame of their own (docs/61). sys_port_spin calls the input pump from the game's own busy-waits, so
 * pumping this edge there catches them mid-screen. Guarded, because shots_poll also runs it at the frame
 * boundary. */
static int g_res_edge_busy = 0;
/* Dense shots while a host-debug warp screen (fn_038f3d / fn_038cde) is up. Such a screen runs entirely
 * inside one host step, so the frame-boundary watchers cannot see it at all; and its own resource announces
 * all happen at screen entry, long before the picture is composited (measured: assets announced at frame
 * ~2778, picture up by frame ~3500, docs/61). A half-second series driven from the spin point covers the
 * screen from entry to exit, so the composited frames exist and are named. */
static int    g_warp_active = 0, g_warp_shot_n = 0;
static Uint64 g_warp_next_shot = 0;
static void warp_shots_poll(void) {
    if (!g_warp_active || !g_shots_dir) return;
    Uint64 now = SDL_GetPerformanceCounter();
    if (now < g_warp_next_shot) return;
    g_warp_next_shot = now + g_perf_freq / 2;
    char nm[72]; snprintf(nm, sizeof nm, "warp_%02d", g_warp_shot_n++);
    shot_named(nm);
}

static void shots_resource_edge(void) {
    if (!g_shots_dir || !g_shot_res || g_res_edge_busy) return;
    if (g_res_trace_seq == g_pt_res_seq) {
        if (getenv("NIGHT_SHOTDBG")) fprintf(stderr, "res edge: seq unchanged (%u)\n", (unsigned)g_res_trace_seq);
        return;
    }
    if (getenv("NIGHT_SHOTDBG")) {   /* the docs/61 probe: is the edge reached with a changed seq? */
        fprintf(stderr, "res edge: name=%s seq=%u\n", g_res_trace, (unsigned)g_res_trace_seq);
    }
    g_res_edge_busy = 1;
    g_pt_res_seq = g_res_trace_seq;
    g_pt_hash = 0;
    shot_named(g_res_trace);
    shot_queue(g_res_trace);
    g_pt_last_scene = SDL_GetPerformanceCounter();
    shot_flush_pending();
    g_res_edge_busy = 0;
}

/* Periodic shots while a still-image card is on screen.  A card is announced by its loader but is only
 * on screen afterwards, and the intro cards hold for well under a second (measured: the stage-1 sequence
 * 1s1/1s2/1s3 announces at frames 2339 / 2378 / 2418, i.e. ~39 frames = 0.7 s each), so the announce-time
 * shot photographs the moment before the blit and cannot be compared with the card itself.  A 200 ms series
 * during a 640x480 view gives every card at least two frames of its own, which scripts/playtest_compare_slides.py
 * then matches to the decoded PCX. */
static Uint64 g_card_next = 0;
static int    g_card_n = 0;
static void card_shots_poll(int w) {
    static int g_dense = -1;
    static int g_dense_hz = -1;          /* NIGHT_DENSEHZ: sample the 320x200 view faster than 10 Hz, up to one
                                          * shot per presented frame (~55 Hz modelled), which is what a "corrupted
                                          * startup frame that heals after a while" report needs: a 10 Hz series can
                                          * step straight over a transient bad frame */
    if (g_dense < 0) { const char *e = getenv("NIGHT_DENSE"); g_dense = (e && e[0] == '1'); }
    if (g_dense_hz < 0) { const char *e = getenv("NIGHT_DENSEHZ"); g_dense_hz = (e && atoi(e) > 0) ? atoi(e) : 10; }
    if (!g_shots_dir) return;
    if (w != 640 && !(g_dense && w == 320)) return;
    Uint64 now = SDL_GetPerformanceCounter();
    if (now < g_card_next) return;
    g_card_next = now + (w == 640 ? g_perf_freq / 5 : g_perf_freq / (Uint64)g_dense_hz);
    char nm[72]; snprintf(nm, sizeof nm, w == 640 ? "card_%02d" : "dense_%02d", g_card_n++);
    shot_named(nm);
}

static void shots_poll(void) {
    if (!g_shots_dir) return;
    shot_flush_pending();
    card_shots_poll(g_vid_w ? g_vid_w : sys_video_w());   /* frames of the card itself, not of its load */
    Uint64 now = SDL_GetPerformanceCounter();
    if (g_shot_res && g_res_trace_seq != g_pt_res_seq) {
        g_pt_res_seq = g_res_trace_seq;
        g_pt_hash = 0;                                  /* a new resource resets the scene reference */
        shot_named(g_res_trace);
        shot_queue(g_res_trace);
        g_pt_last_scene = now;
        shot_flush_pending();
        return;
    }
    if (g_shot_fli && strstr(g_res_trace, ".fli")) {
        if (now >= g_pt_last_fli + g_perf_freq / 2) {
            g_pt_last_fli = now;
            shot_named("fli");
        }
        return;
    }
    if (!g_shot_scene) return;
    uint64_t hash = 1469598103934665603ull;
    int diff = 0, ns = 0;
    for (int i = 0; i < 307200 && ns < (int)sizeof g_pt_sample; i += 193) {
        uint8_t v = g_video_ram[i];
        hash = (hash ^ v) * 1099511628211ull;
        if (g_pt_hash && g_pt_sample[ns] != v) diff++;
        g_pt_sample[ns++] = v;
    }
    if (g_pt_hash && hash != g_pt_hash && diff > ns / 4 && now >= g_pt_last_scene + g_perf_freq / 2) {
        g_pt_hash = hash;
        g_pt_last_scene = now;
        g_scene_gen++;
        shot_named("scene");
    } else if (!g_pt_hash) {
        g_pt_hash = hash;
    }
}

static void dump_frame(int n) {
    if (!g_dump_dir) return;
    SDL_CreateDirectory(g_dump_dir);          /* so --dump to a fresh path works */
    int W = g_vid_w ? g_vid_w : sys_video_w(), H = g_vid_h ? g_vid_h : sys_video_h();
    palette_rgba();
    uint8_t seen[256]; memset(seen, 0, sizeof seen);
    for (int i = 0; i < W * H; i++) seen[g_video_ram[i]] = 1;
    int ndx = 0, lit = 0, dac_max = 0, dac_gt63 = 0;
    for (int i = 0; i < 256; i++) if (seen[i]) ndx++;
    for (int i = 0; i < 768; i++) { if (g_palette8[i] > dac_max) dac_max = g_palette8[i]; if (g_palette8[i] > 63) dac_gt63++; }
    for (int i = 0; i < 768; i += 3) if (g_palette8[i] | g_palette8[i + 1] | g_palette8[i + 2]) lit++;
    SDL_Log("frame %d: %dx%d colours=%d  dac lit=%d  dac writes=%u  "
            "PIT0=%u Hz tick=%u DAT_63e40=%u DAT_633a0=%u stage=%u W=%u pitch=%u  "
            "room=%p entities=%d list=%p dacmax=%d dacle64=%d",
            n, W, H, ndx, lit, sys_vga_palette_writes(),
            sys_pit_hz(0), g_dat_63e24, g_dat_63e40, g_dat_633a0, g_dat_63370,
            g_dat_531f4, g_dat_63e64, (void *)g_dat_6334c, (int)g_dat_53c68, (void *)g_dat_53c70,
            dac_max, dac_gt63);
    {
        char pb[512]; int po = 0;
        SDL_Log("board_out pushes=%u bytes=%u buf=%p hdr=%u/%u/%u %u/%u/%u %u/%u/%u",
                g_board_out_pushes, g_board_out_bytes, (void *)g_board_out_lastbuf,
                g_board_out_hdr[0],g_board_out_hdr[1],g_board_out_hdr[2], g_board_out_hdr[3],g_board_out_hdr[4],g_board_out_hdr[5],
                g_board_out_hdr[6],g_board_out_hdr[7],g_board_out_hdr[8]);
        for (int k = 0; k < BOC_TRACE_N; k++)
            if (g_board_out_ra[k])
                SDL_Log("  board_out caller +%#lx x%u", (unsigned long)(g_board_out_ra[k] - (uintptr_t)(void *)board_out), g_board_out_ra_n[k]);
        SDL_Log("DAT_63e50[0..5] shift=%u: %08x %08x %08x %08x %08x %08x", g_dat_63e4c,
                g_dat_63e50[0],g_dat_63e50[1],g_dat_63e50[2],g_dat_63e50[3],g_dat_63e50[4],g_dat_63e50[5]);
        SDL_Log("staging DAT_633b8[0..9]: %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
                g_dat_633b8[0],g_dat_633b8[1],g_dat_633b8[2], g_dat_633b8[3],g_dat_633b8[4],g_dat_633b8[5],
                g_dat_633b8[6],g_dat_633b8[7],g_dat_633b8[8], g_dat_633b8[9],g_dat_633b8[10],g_dat_633b8[11],
                g_dat_633b8[12],g_dat_633b8[13],g_dat_633b8[14]);
        for (int i = 0; i < 10; i++)
            po += snprintf(pb + po, sizeof pb - po, "%u/%u/%u ", g_palette8[i*3], g_palette8[i*3+1], g_palette8[i*3+2]);
        SDL_Log("dac palette[0..9]: %s", pb);
        {
            int dark = 0, mid = 0, hi = 0, white = 0;
            for (int i = 0; i < 256; i++) {
                int mx = g_palette8[i*3], mn = g_palette8[i*3];
                for (int k = 1; k < 3; k++) { if (g_palette8[i*3+k] > mx) mx = g_palette8[i*3+k]; if (g_palette8[i*3+k] < mn) mn = g_palette8[i*3+k]; }
                if (mx < 32) dark++; else if (mx < 96) mid++; else if (mx < 192) hi++; else white++;
                (void)mn;
            }
            SDL_Log("palette buckets dark<32=%d mid<96=%d hi<192=%d white=%d ; [100..103]: %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u  ; idx writes=%u data writes=%u",
                    dark, mid, hi, white,
                    g_palette8[300],g_palette8[301],g_palette8[302], g_palette8[303],g_palette8[304],g_palette8[305],
                    g_palette8[306],g_palette8[307],g_palette8[308], g_palette8[309],g_palette8[310],g_palette8[311],
                    sys_outb_count(0x3c8), sys_outb_count(0x3c9));
        }
        {
            int idx[6] = { 0x3c6, 0x3c8, 0x3c9, 0x20, 0x21, 0x61 };
            char ob[256]; int oo = 0;
            for (int k = 0; k < 6; k++) oo += snprintf(ob + oo, sizeof ob - oo, "%03x=%u ", idx[k], sys_outb_count((uint16_t)idx[k]));
            /* top-6 written ports overall */
            for (int rep = 0; rep < 0; rep++);
            SDL_Log("outb counts: %s", ob);
        }
    }
    SDL_Log("screen base DAT_633b0=%p delta_from_video_ram=%td  DAT_63354=%p",
            (void *)g_dat_633b0, (ptrdiff_t)((uint8_t *)g_dat_633b0 - g_video_ram), (void *)g_dat_63354);
    char path[512];
    snprintf(path, sizeof path, "%s/frame%05d.bmp", g_dump_dir, n);
    SDL_Surface *s = SDL_CreateSurface(W, H, SDL_PIXELFORMAT_RGBA8888);
    if (!s) return;
    uint8_t *dst = (uint8_t *)s->pixels;
    /* RGBA8888 is a packed 32-bit format (R<<16|G<<8|B, A<<24), so write whole pixels: byte-wise
     * fills land the channels in the wrong slots and every dump came out with a saturated red. */
    for (int i = 0; i < W * H; i++) {
        uint8_t idx = g_video_ram[i];
        ((uint32_t *)dst)[i] = ((uint32_t)g_palette8[idx * 3] << 24) |
                               ((uint32_t)g_palette8[idx * 3 + 1] << 16) |
                               ((uint32_t)g_palette8[idx * 3 + 2] << 8) | 0xffu;
    }
    SDL_SaveBMP(s, path);
    {   /* raw side dump: the palette mirror and the first 64000 bytes of the screen, for offline diffing */
        char rp[540]; snprintf(rp, sizeof rp, "%s/pal%05d.bin", g_dump_dir, n);
        FILE *fp = fopen(rp, "wb"); if (fp) { fwrite(g_palette8, 1, 768, fp); fclose(fp); }
        snprintf(rp, sizeof rp, "%s/vidscr%05d.bin", g_dump_dir, n);
        fp = fopen(rp, "wb"); if (fp) { fwrite(g_screen_ram, 1, 0xfa00, fp); fclose(fp); }
        snprintf(rp, sizeof rp, "%s/palsrc%05d.bin", g_dump_dir, n);
        fp = fopen(rp, "wb"); if (fp) { fwrite(g_dat_636b8, 1, 768, fp); fclose(fp); }
        snprintf(rp, sizeof rp, "%s/palrot%05d.bin", g_dump_dir, n);
        fp = fopen(rp, "wb"); if (fp) { fwrite(g_dat_633b8, 1, 768, fp); fclose(fp); }
        snprintf(rp, sizeof rp, "%s/vid%05d.bin", g_dump_dir, n);
        fp = fopen(rp, "wb"); if (fp) { fwrite(g_video_ram, 1, 307200, fp); fclose(fp); }
    }
    /* The room/HUD composite target is the back screen DAT_633b0 (320x200, pitch 0x140), not the
     * VGA linear arena; dump it separately so the flip question is answerable. */
    snprintf(path, sizeof path, "%s/frame%05d_scr.bmp", g_dump_dir, n);
    {
        SDL_Surface *ss = SDL_CreateSurface(320, 200, SDL_PIXELFORMAT_RGBA8888);
        if (ss) {
            uint8_t *d = (uint8_t *)ss->pixels;
            for (int i = 0; i < 320 * 200; i++) {
                uint8_t idx = g_screen_ram[i];
                ((uint32_t *)d)[i] = ((uint32_t)g_palette8[idx*3] << 24) |
                                     ((uint32_t)g_palette8[idx*3+1] << 16) |
                                     ((uint32_t)g_palette8[idx*3+2] << 8) | 0xffu;
            }
            SDL_SaveBMP(ss, path);
            SDL_DestroySurface(ss);
        }
    }
    SDL_DestroySurface(s);
    /* The 320x200 view: the game's own screens and its RLE picture loader are 320 wide (pitch 0x140,
     * 0xfa00 bytes), so this is the view that shows what the loaders actually painted. */
    snprintf(path, sizeof path, "%s/frame%05d_320.bmp", g_dump_dir, n);
    SDL_Surface *s2 = SDL_CreateSurface(320, 200, SDL_PIXELFORMAT_RGBA8888);
    if (s2) {
        uint8_t *d2 = (uint8_t *)s2->pixels;
        for (int i = 0; i < 320 * 200; i++) {
            uint8_t idx = g_video_ram[i];
            ((uint32_t *)d2)[i] = ((uint32_t)g_palette8[idx * 3] << 24) |
                                  ((uint32_t)g_palette8[idx * 3 + 1] << 16) |
                                  ((uint32_t)g_palette8[idx * 3 + 2] << 8) | 0xffu;
        }
        SDL_SaveBMP(s2, path);
        SDL_DestroySurface(s2);
    }
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
        case SDL_SCANCODE_ESCAPE:  sc = 0x01; break;   /* docs/93: the shipped room loop turns the abort byte into fn_039c03 (the pause/setup menu) -- the host must NOT quit (docs/60) */
        case SDL_SCANCODE_SPACE:   sc = 0x39; break;
        case SDL_SCANCODE_F1:      sc = NIGHT_SC_F1; break;   /* docs/91: sets DAT_00063ec3, the level-up help pages */
        case SDL_SCANCODE_LCTRL:   sc = NIGHT_SC_CTRL; break;
        case SDL_SCANCODE_RCTRL:   sc = NIGHT_SC_CTRL; break;   /* E0 1D: the handler lands on the same slot (measured) */
        case SDL_SCANCODE_LSHIFT:  sc = NIGHT_SC_LSHIFT; break;
        case SDL_SCANCODE_RSHIFT:  sc = NIGHT_SC_RSHIFT; break; /* its own slot +0x36, held byte DAT_00063ebe (measured) */
        case SDL_SCANCODE_LALT:    sc = NIGHT_SC_ALT; break;
        case SDL_SCANCODE_RALT:    sc = NIGHT_SC_ALT; break;    /* E0 38 (measured) */
        case SDL_SCANCODE_Z:       sc = NIGHT_SC_Z; break;      /* punch alias -> CTRL, handler 0x0003fdd1 */
        case SDL_SCANCODE_X:       sc = NIGHT_SC_X; break;      /* kick alias -> ALT */
        case SDL_SCANCODE_C:       sc = NIGHT_SC_C; break;      /* jump alias -> SHIFT */
        case SDL_SCANCODE_V:       sc = NIGHT_SC_V; break;
        case SDL_SCANCODE_B:       sc = NIGHT_SC_B; break;
        default: break;
        }
        if (!sc) continue;
        key_apply(sc, ev.key.down);
    }
}

/* the game-side half of input handling: SDL key events and the playtest script both land here, so a
 * scripted run drives exactly the bytes an interactive run does */
/* The game-side half of input handling: SDL key events and the playtest script both land here, so a
 * scripted run drives exactly the bytes an interactive run does. The mapping itself lives in
 * src/app/keymap.c (docs/91) because the test binary asserts it against the values measured on the original. */
static void key_apply(uint8_t sc, int make) { night_key_apply(sc, make); }

static void input_pump(void) {
    autokey();                                                 /* the scripted headless key press */
    pt_drive();                                                /* the playtest script's key timeline */
    if (g_rig_dir) rig_poll(pt_now_ms(), (unsigned long long)g_frames,
                           (unsigned long long)(g_astream ? g_audio_submitted : g_audio_drained));
    shots_resource_edge();   /* names announced inside a nested screen are only visible from a spin point */
    warp_shots_poll();       /* dense frames while a host-warp screen is up */
    card_shots_poll(g_vid_w ? g_vid_w : sys_video_w());   /* the slide show runs inside one host step, so
                                                           * the frame-boundary watcher never samples it */
    /* A screen whose loop blocks inside a single game step (the ending slide show, the slide shows, the CG
     * picture show) never reaches the frame-boundary present(): the window sat on the previous scene for the
     * whole screen -- and every shot taken from a spin point kept the stale geometry, because g_vid_w is only
     * updated inside ensure_mode. Present from the cooperative pump too, on the same 60 Hz throttle. */
    if (g_ren) {
        static Uint64 s_pump_present_next;                    /* its own throttle: reusing g_present_next here
                                                               * starved the main loop's present branch, and that
                                                               * branch is where g_frames advances -- the frame
                                                               * counter froze at 1 and every frame-gated script,
                                                               * warp and --frames budget stopped firing */
        Uint64 np = SDL_GetPerformanceCounter();
        if (np >= s_pump_present_next) {
            present();
            s_pump_present_next = np + g_perf_freq / 60u;
        }
    }
    /* A finished script means the run has produced everything it was going to: end the process normally so
     * the manifest is flushed (killing it loses the tail). */
    if (g_pt_done_ms >= 0 && pt_now_ms() - g_pt_done_ms > 2500) g_quit = 1;
    sdl_events();
    fn_0347e3();                                           /* the game's keyboard pump */
}

/* ---- the tick pump: real time -> the installed timer handler ---------- */
/* Which per-type sprite/action set each live entity actually uses: entity+0x123 holds the act-table
 * value copied from g_dat_53818[type] by the constructor (fn_010efc), so matching it back against the
 * table names the entity's type -- the direct way to see whether every enemy is landing on type 0
 * ("gun", i.e. the protagonist's art). */
static void ent_dump(void) {
    int n = 0;
    for (uint32_t i = 0; i < 0x65; i++) if (g_dat_53818[i]) n++;
    SDL_Log("ent: count=%d tables loaded=%d cameraX=%d cameraY=%d playerBlock=%p",
            (int)g_dat_53c68, n, (int)g_dat_639dc, (int)g_dat_639e0, (void *)g_player_ent);
    uint8_t *e = g_dat_53c70;
    for (int k = 0; k < 8 && e; k++) {
        uint32_t act = 0, off = 0;
        memcpy(&act, e + 0x123, 4);
        int ty = -1;
        for (uint32_t i = 0; i < 0x65; i++) if (g_dat_53818[i] == act && act) { ty = (int)i; break; }
        memcpy(&off, e + 0x127, 4);
        int sty = -1;
        for (uint32_t i = 0; i < 0x65; i++)
            if (g_dat_639ec[i] && memcmp(g_dat_639ec[i] + 0x38, e + 0x127, 4) == 0) { sty = (int)i; break; }
        uint32_t x = 0, y = 0, st = 0, face = 0;
        memcpy(&x, e + 0x01, 4); memcpy(&y, e + 0x09, 4);      /* world x/y (see fn_011827) */
        memcpy(&st, e + 0x17, 4); memcpy(&face, e + 0x1b, 4);
        SDL_Log("  ent[%d] block=%p type(act)=%d type(spr)=%d world=%u,%u state=0x%x face=%u isPlayer=%d "
                "sprfields=%08x %08x %08x",
                k, (void *)e, ty, sty, x, y, st, face, e == g_player_ent,
                ((struct ua32 *)((e + 0x127)))->v, ((struct ua32 *)((e + 0x12b)))->v, ((struct ua32 *)((e + 0x12f)))->v);
        /* Machine-readable form of the same record, with the shipped asset each type resolves to: this is what
         * scripts/playtest_check_entity_sprites.py asserts on (docs/73). */
        {
            const char *const *sn = f_type_spr_names();
            const char *const *an = f_type_act_names();
            SDL_Log("entmap idx=%d player=%d type_act=%d type_spr=%d spr_asset=%s act_asset=%s "
                    "rbase=%08x spr_struct=%p",
                    k, e == g_player_ent ? 1 : 0, ty, sty,
                    (ty >= 0 && ty < 0x65) ? sn[ty] : "?", (ty >= 0 && ty < 0x65) ? an[ty] : "?",
                    ((struct ua32 *)((e + 0x127)))->v, (void *)(sty >= 0 ? g_dat_639ec[sty] : NULL));
        }
        e = ((struct ua_uint8tp *)((e + 0x13f)))->v;
    }
}

static Uint64 g_tick_next;
static unsigned long g_pump_calls, g_timer_calls;
static Uint64 g_diag_next;
static Uint64 g_audio_next;
static void tick_pump(void) {
    Uint64 now = SDL_GetPerformanceCounter();
    g_pump_calls++;
    if (now >= g_audio_next) {                 /* feed the sound stream on a 20 ms cadence */
        g_audio_next = now + g_perf_freq / 50;
        audio_pump();
        cdda_pump();
    }
    if (now >= g_diag_next) {                      /* progress diagnostics: is the clock moving? */
        SDL_Log("pump: calls=%lu timer_calls=%lu frames=%u tick=%u DAT_63e40=%u DAT_63e3c=%u "
                "pit=%u Hz retrace=%lu audio=%uHz pushed=%u pend=%u xfer=%u stops=%u cdda=%lluB tracks=%u pos=%u snden=%u devinit=%u plays=%u real=%lu null=%lu cdstops=%u sfx=%u keyb_ed0_ed3_ed5_ed8=%u/%u/%u/%u "
                "vgaws=%u vga_bytes=%llu wipe_keys_ea4_ec1_ea5_ec0=%u/%u/%u/%u tbl53eb8=%u/%u",
                g_pump_calls, g_timer_calls, g_frames, g_dat_63e24, g_dat_63e40, g_dat_63e3c,
                sys_pit_hz(0), (unsigned long)g_vga_retrace_waits,
                sys_audio_rate(), sys_audio_pushed(), sys_audio_pending(),
                sys_audio_transfers(), sys_audio_stops(),
                (unsigned long long)g_cdda_bytes, (unsigned)g_cdda_tracks, (unsigned)g_cdda_pos,
                (unsigned)g_dat_53338, (unsigned)g_dat_63f90, (unsigned)g_play_calls, g_play_real, g_play_null,
                /* docs/91: the arg order had drifted out of step with the labels above -- cdstops/sfx/keyb/vgaws/
                 * vga_bytes/wipe_keys were printing each other's values (the giveaway was vga_bytes=1 next to a
                 * nonsense sfx=1124889600, which is the real 64000-byte-per-flip counter). Re-ordered to match. */
                (unsigned)sys_cd_audio_stop_calls(), (unsigned)g_dat_51a10,
                (unsigned)g_dat_63ed0, (unsigned)g_dat_63ed3, (unsigned)g_dat_63ed5, (unsigned)g_dat_63ed8,
                g_vga_window_ns, (unsigned long long)g_vga_window_bytes,
                (unsigned)g_dat_63ea4, (unsigned)g_dat_63ec1, (unsigned)g_dat_63ea5, (unsigned)g_dat_63ec0,
                (unsigned)g_tbl53eb8_max_idx, (unsigned)g_tbl53eb8_clamped);
        /* Host debug warp for screens a scripted route cannot reach in the time a run lasts: the level-up
     * (fn_038f3d, variant 1 -- the game itself calls it at 0x0003b54c right after g_dat_63370 += 1 on stage
     * completion, guarded by pl[0x30] != 1) and the status (variant 0) / help (fn_038cde, the pause menu's
     * entries 0 and 1 at 0x0003d2ac / 0x0003d2c1) screens. The warp calls the game's own routine, so the
     * asset loads, the compositing and the key handling under test are the shipped ones; only the moment of
     * entering them is host-chosen (same convention as --warp-stage, docs/52).
     * NIGHT_SHOWSCREEN=levelup|status|help, NIGHT_SHOWSCREEN_AT=<frame>. */
    if (getenv("NIGHT_MENUBG")) g_menu_trace = 1;
    if (getenv("NIGHT_KEYTRACE")) g_key_trace = 1;
    if (getenv("NIGHT_ACTTRACE")) g_act_trace = 1;
    if (getenv("NIGHT_HITTRACE")) g_hit_trace = 1;
    if (getenv("NIGHT_CHAINTRACE")) g_chain_trace = 1;
    if (getenv("NIGHT_HUDDBG")) g_hud_trace = 1;
    if (getenv("NIGHT_ROOMDBG")) g_room_trace = 1;   /* stage room-chain witness (docs/90) */
    {   /* docs/88: drive the credit-less game-over branch the shipped way -- write the game's own state while
         * the room loop is live. DAT_00053100 is the CONTINUE credit count (init 3) and DAT_0006336c == 1 is the
         * "gone for good" latch the room loop tests at 0x0003abf1; the loop's post-break branch then runs
         * `if (DAT_00053100 == 0) { DAT_00053100 = 3; return 0; }` at 0x0003aeb7-0x0003aec8, the path docs/81
         * never reached because it kept one credit left. */
        static int s_credits_done;
        const char *cr = getenv("NIGHT_CREDITS");
        if (cr && !s_credits_done) {
            s_credits_done = 1;
            g_dat_53100 = (uint32_t)atoi(cr);
            SDL_Log("credits warp: DAT_53100 = %s", cr);
        }
        static int s_quit_fired, s_quit_defer_logged;
        const char *qa = getenv("NIGHT_QUIT_AT");
        if (qa && !s_quit_fired && (long)g_frames >= atol(qa)) {
            if (g_dat_63354 == 0) {          /* same precondition style as fn_showscreen_gate (docs/78) */
                if (!s_quit_defer_logged) {
                    s_quit_defer_logged = 1;
                    SDL_Log("quit warp: deferred at frame %u: no live room record (the room loop owns the latch)",
                            (unsigned)g_frames);
                }
            } else {
                s_quit_fired = 1;
                g_dat_6336c = 1;
                SDL_Log("quit warp: DAT_6336c = 1 at frame %u (credits=%u)", (unsigned)g_frames, g_dat_53100);
            }
        }
    }
        { static int s_shown;                                  /* the debug warp for the menu screens */
          const char *what = getenv("NIGHT_SHOWSCREEN");
          long at = getenv("NIGHT_SHOWSCREEN_AT") ? atol(getenv("NIGHT_SHOWSCREEN_AT")) : 4000;
          static int s_defer_logged;
          if (what && !s_shown && (long)g_frames >= at) {
              /* docs/78: the shipped screen routines carry the preconditions their in-game call sites guarantee.
               * fn_038445 opens by fading the CURRENT ROOM's palette out of DAT_63354+0x48, so warping to it
               * before any room is loaded dereferences 0x48 (ASan SEGV). Defer instead of firing. */
              const char *why = fn_showscreen_gate(what);
              if (why) {
                  if (!s_defer_logged) { s_defer_logged = 1;
                      SDL_Log("showscreen warp: %s deferred at frame %u: %s", what, g_frames, why); }
              } else {
              s_shown = 1;
              SDL_Log("showscreen warp: %s at frame %u", what, g_frames);
              g_warp_active = 1; g_warp_next_shot = SDL_GetPerformanceCounter() + g_perf_freq / 2;
              if (!strcmp(what, "levelup"))      fn_038f3d(1);
              else if (!strcmp(what, "status"))  fn_038f3d(0);
              else if (!strcmp(what, "help"))    fn_038cde();
              else if (!strcmp(what, "gameover")) fn_038445();   /* the game-over / CONTINUE loop (docs/61
                                                                  * convention; its only in-game call site is
                                                                  * 0x0003aecf inside fn_03af18) */
              else if (!strcmp(what, "ending")) fn_03af18_ending_slideshow();
                                                                  /* the "game is through" ending slide show
                                                                   * (LAB_0003b2b1, docs/64): in-game it is only
                                                                   * reachable by clearing episode 9, which scripted
                                                                   * play does not do */
              else SDL_Log("showscreen warp: unknown screen '%s'", what);
              g_warp_active = 0;
              }
          } }
    /* Host-side debug warp (docs/84): NIGHT_WARP_ENTITY=enc-1,boss1,dust asks for shipped entity
     * types in the live room, which is how the sprite-family census reaches the families scripted play
     * never clears its way to. The name -> type map is the game's own per-type name table, the spawn
     * is the shipped room-record opcode-1 shape (fn_00041bdc + fn_010efc), and the request is deferred
     * (with the reason logged) while the shipped preconditions are unmet: no live room record, or this
     * episode's stage records never loaded the type's .spr/.act blocks. */
    { static char wnames[24][0x20]; static int wnn = -1, wdone[24], wdefer[24];
      if (wnn < 0) {
          const char *e = getenv("NIGHT_WARP_ENTITY");
          wnn = 0;
          const char *p = e;
          while (p && *p && wnn < 24) {
              const char *q = strchr(p, ',');
              size_t n = q ? (size_t)(q - p) : strlen(p);
              if (n >= sizeof wnames[0]) n = sizeof wnames[0] - 1;
              memcpy(wnames[wnn], p, n); wnames[wnn][n] = 0; wnn++;
              p = q ? q + 1 : NULL;
          }
      }
      if (wnn > 0) {
          long at = getenv("NIGHT_WARP_ENTITY_AT") ? atol(getenv("NIGHT_WARP_ENTITY_AT")) : 0;
          if ((long)g_frames >= at) {
              for (int i = 0; i < wnn; i++) {
                  if (wdone[i]) continue;
                  const char *why = f_warp_entity_refuse(wnames[i]);
                  if (why) {
                      if (!wdone[i] && (long)g_frames < at + 700 && ++wdefer[i] == 1)
                          fprintf(stderr, "warp-entity: %s deferred at frame %u: %s\n",
                                  wnames[i], g_frames, why);
                      continue;
                  }
                  uint8_t *ent = f_warp_entity_spawn(wnames[i]);
                  wdone[i] = 1;
                  fprintf(stderr, "warp-entity: %s type=%u frame=%u %s\n", wnames[i],
                          (unsigned)f_warp_entity_type(wnames[i]), g_frames,
                          ent ? "spawned" : "allocation failed");
              }
          }
      }
    }
    if (getenv("NIGHT_ENTDBG")) ent_dump();
        if (getenv("NIGHT_BLITSTATS")) blit_stats_dump();
        if (getenv("NIGHT_HEAPDUMP")) { static int s_hd; if (!s_hd && g_dat_63360 >= 900 && !s_hd++) {
            FILE *fp = fopen("/tmp/heap.bin", "wb");
            if (fp) { fwrite((const void *)g_heap_base, 1, heap_arena_cap_get(), fp); fclose(fp); }
            SDL_Log("heap dump: base=%p cap=%#x", (void *)g_heap_base, heap_arena_cap_get());
        } }
        {   /* docs/91 soak witness: arena take-up and RSS, tagged with the stage in play.
             * getrusage rather than SDL_GetProcessMemoryInfo: it is in every SDL3 we build against, and
             * ru_maxrss is KiB on Linux and bytes on macOS -- the unit is named in the log line. */
            struct rusage ru;
            if (getrusage(RUSAGE_SELF, &ru) == 0) {
#if defined(__APPLE__)
                long long rss_kb = (long long)ru.ru_maxrss / 1024; const char *unit = "KB";
#else
                long long rss_kb = (long long)ru.ru_maxrss; const char *unit = "KB";
#endif
                (void)unit;
                SDL_Log("memstat stage=%u room=%u frames=%u arena=%u/%u rss_max=%lld KB",
                        (unsigned)g_dat_63370, (unsigned)(g_dat_63354 != 0), (unsigned)g_frames,
                        heap_arena_used(), heap_arena_cap_get(), rss_kb);
            }
        }
        g_diag_next = now + g_perf_freq;
    }
    if (now < g_tick_next && !g_fixed_step_calls) return;
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
    if (g_rig_ctl & 1) {
        g_pump_calls++;                                /* the pump runs; the shipped tick handler does not */
    } else if (g_fixed_step_calls) {
        /* Fixed step (NIGHT_RIG_STEP_CALLS, docs/107): the PIT handler advances a CONSTANT number of
         * ticks per presented frame instead of following the wall clock. Wall-clock pumping made back-to-
         * back runs of one drive diverge (38 hits vs 5 hits on ep1_combat with the same LCG seed), because
         * a host that falls behind re-anchors and drops ticks -- so keys landed on different game ticks.
         * With a fixed step the tick sequence is a function of the presented frame count alone, which is
         * what replayable fuzzing and minimal repros need. 8 = 490 Hz / ~60 fps. */
        static unsigned long s_step_frames;
        if (g_frames != s_step_frames) {
            s_step_frames = g_frames;
            unsigned long want = (unsigned long)((unsigned long long)g_frames *
                                                 (unsigned long long)g_fixed_step_calls /
                                                 (unsigned long long)g_fixed_fps);
            while (g_timer_calls < want && g_timer_calls < s_step_frames * 64ul + 64ul) {
                fn_03ee88(); g_timer_calls++;
            }
        }
    } else {
        while (calls--) { fn_03ee88(); g_timer_calls++; }
    }

    if (now >= g_present_next) {
        present();
        g_frames++;
        /* docs/146 §9: `--frames N` was only a floor. The pumped frame counter is checked *after* the game's
         * own main loop unwinds (main.c:1560), so a run asked for 300 frames still played the whole episode:
         * measured, `--frames 300` presented 11412 frames and took 221 s, because presentation is paced to
         * wall time at 60 fps. Every wall-clock cap in the fuzz corpus was therefore cutting runs at an
         * unpredictable point -- which is what docs/120 §6-§8 saw as rc=124 seeds, and why coverage per seed
         * was not comparable. N is now a budget the game actually stops at. */
        if (g_frame_budget && (int)g_frames >= g_frame_budget) {
            if (!g_quit) {
                g_budget_hits++;
                fprintf(stderr, "frame budget: %d presented, unwinding the game main\n", g_frame_budget);
            }
            g_quit = 1;
        }
        for (unsigned k = 0; k < sizeof g_dump_frames / sizeof g_dump_frames[0]; k++)
            if (g_frames == g_dump_frames[k]) dump_frame(g_frames);
        shots_poll();                                       /* named-event playtest screenshots */
        g_present_next = now + g_perf_freq / 60u;
    }
    if (g_quit) longjmp(g_game_escape, 1);
}

int main(int argc, char **argv) {
    if (getenv("NIGHT_CENSUS")) {
        rig_census_begin();
        const char *rd = NULL;
        for (int i = 1; i + 1 < argc; i++) if (!strcmp(argv[i], "--rig-dump")) rd = argv[i + 1];
        char cp[600];
        snprintf(cp, sizeof cp, "%s/census.txt", rd ? rd : "/tmp");
        rig_census_set_path(cp);
    }
    const char *res_dir = "../NIGHT";
    int frames_limit = 0;
    /* NIGHT_DELAY_US re-times the modelled cost of one guest DOS clock read (see delay_units in f.c); 0
     * restores the timing-free behaviour. Default 500 us, measured against the original. */
    const char *e_delay = getenv("NIGHT_DELAY_US");
    if (e_delay) g_delay_unit_us = (uint32_t)atoi(e_delay);
    /* NIGHT_VGAWS sets the modelled cost of one byte written through the A0000 VGA window, in ns. Default 98:
     * the main title's wipe is 240 passes x 307200 window bytes and the original's own frames put that wipe at
     * 7.19 s, i.e. 29.96 ms per pass = 97.5 ns/byte (two 486 cycles per uncacheable write; docs/72). The DEFAULT is
     * lower than that physical figure on purpose: charged per scanline through the pump, each wait overshoots by
     * tens of microseconds, and measured end to end 98 ns/byte gave 11.13 s (ratio 1.55), so the model is tuned so
     * the observed span lands on the original's 7.19 s. 0 disables. */
    const char *e_vgaws = getenv("NIGHT_VGAWS");
    g_vga_window_ns = e_vgaws ? (uint32_t)atoi(e_vgaws) : 63u;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--res-dir") && i + 1 < argc) res_dir = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) {
            frames_limit = atoi(argv[++i]);
            g_frame_budget = frames_limit;   /* docs/146 §9 */
        }
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) g_dump_dir = argv[++i];
        else if (!strcmp(argv[i], "--autokey")) g_autokey = 1;
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) pt_load(argv[++i]);
        else if (!strcmp(argv[i], "--no-audio")) g_no_audio = 1;
        else if (!strcmp(argv[i], "--shots") && i + 1 < argc) g_shots_dir = argv[++i];
        else if (!strcmp(argv[i], "--shot-limit") && i + 1 < argc) g_shot_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--shot-delay") && i + 1 < argc) g_shot_delay_ms = (Uint64)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--warp-stage") && i + 1 < argc) g_warp_stage = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--rig-dump") && i + 1 < argc) g_rig_dir = argv[++i];
        else if (!strcmp(argv[i], "--rig-hz") && i + 1 < argc) g_rig_hz = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--no-scene-shots")) g_shot_scene = 0;
        else if (!strcmp(argv[i], "--no-res-shots")) g_shot_res = 0;
        else if (!strcmp(argv[i], "--help")) {
            printf("usage: night [--res-dir DIR] [--frames N] [--dump DIR] [--shots DIR]\n"
                   "              [--script FILE] [--shot-limit N] [--shot-delay MS] [--no-audio] [--no-scene-shots] [--no-res-shots]\n");
            return 0;
        }
    }
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--resnames")) g_resnames = 1;
    if (getenv("NIGHT_FLIDBG")) g_fli_trace = (uint32_t)atoi(getenv("NIGHT_FLIDBG"));   /* CG op trace */
    if (getenv("NIGHT_FLIDUMP")) { snprintf(g_fli_dump, sizeof g_fli_dump, "%s", getenv("NIGHT_FLIDUMP")); }
    if (getenv("NIGHT_RES_DIR") && *getenv("NIGHT_RES_DIR")) res_dir = getenv("NIGHT_RES_DIR");
    char cue[1024];
    snprintf(cue, sizeof cue, "%s/TG_NIGHT.CUE", res_dir);

    if (getenv("NIGHT_RETRACE_HZ")) {          /* the 0x3DA retrace model (docs/47) */
        int hz = atoi(getenv("NIGHT_RETRACE_HZ"));
        if (hz > 0) g_vga_retrace_us = (uint32_t)(1000000 / hz);
        SDL_Log("retrace model: %d Hz (%u us per wait)", hz, g_vga_retrace_us);
    }
    if (getenv("NIGHT_PAL")) {
        if (!strcmp(getenv("NIGHT_PAL"), "staging")) g_pal_src = 1;
        else if (!strcmp(getenv("NIGHT_PAL"), "cache")) g_pal_src = 2;
        else if (!strcmp(getenv("NIGHT_PAL"), "cacheraw")) g_pal_src = 3;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    { const char *be = getenv("NIGHT_BOARD"); if (!be || strcmp(be, "0") != 0) board_boot(); }   /* the original ships with the board live (DAT_53334=0x220 at the title, docs/98); NIGHT_BOARD=0 opts out */
    if (getenv("NIGHT_SOUND")) g_dat_53338 = 1;  /* the title menu's SOUND toggle (fn_039c03) is off by
                                                  * default in the shipped image; force it on for tests */
    sys_set_res_hook(reslog_add);
    if (!g_no_audio) audio_boot();
    else SDL_Log("audio: skipped (--no-audio); the board's DMA ring is still drained every frame");
    g_dbg_03d606 = getenv("NIGHT_DBG") ? 1 : 0;
    g_heap_chain_dbg = getenv("NIGHT_DBG") ? 1 : 0;
    if (getenv("NIGHT_BLITSTATS")) blit_stats_on();
    g_win = SDL_CreateWindow("NIGHT", 640 * 2, 480 * 2, 0);
    g_ren = g_win ? SDL_CreateRenderer(g_win, "gpu,metal,software") : NULL;
    if (!g_ren) { SDL_Log("no renderer: %s", SDL_GetError()); return 1; }
    g_perf_freq = SDL_GetPerformanceFrequency();
    if (g_rig_dir) { rig_pcm_open(g_rig_dir); rig_screen_init(g_rig_dir, 10); rig_init(g_rig_dir, g_rig_hz); SDL_Log("rig: dumping %d DS cells to %s at %d Hz",
                        rig_cell_count(), g_rig_dir, g_rig_hz); }

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
        sys_cd_set_audio_backend(cdda_play, cdda_stop);
        SDL_Log("disc: %u tracks, %u sectors, lead-out MSF-derived", cd_iso_track_count(), cd_iso_total_sectors());
    } else {
        SDL_Log("disc: %s not readable -- the disc-present gate will ask for the CD", cue);
    }
    if (g_resnames) {                        /* the expanded name->value index (fn_0366a4) */
        for (uint32_t i = 0; i < g_dat_633a0 && i < 512; i++) {
            const uint8_t *e = &g_tab_5d884[i * 0x15];
            char nm[13]; memcpy(nm, e, 12); nm[12] = 0;
            SDL_Log("res[%u] %-12s value=%u offset=%u", i, nm,
                    ((struct ua32 *)((e + 0x0d)))->v, ((struct ua32 *)((e + 0x11)))->v);
        }
    }
    sys_set_input_pump(input_pump);
    rig_ctl_init();
    sys_set_tick_pump(tick_pump);

    g_start_time = SDL_GetPerformanceCounter();
    g_tick_next = g_start_time;
    g_present_next = g_start_time;
    g_diag_next = g_tick_next + g_perf_freq;

    /* ---- run ---- */
    if (setjmp(g_game_escape) == 0) {
        g_escape_armed = 1;
    { const char *st = getenv("NIGHT_RIG_STEP_CALLS"); if (st) g_fixed_step_calls = atoi(st);
      const char *fp = getenv("NIGHT_RIG_FPS"); if (fp) g_fixed_fps = atol(fp);
      if (g_fixed_step_calls) SDL_Log("fixed step: %d PIT Hz / %ld fps -- game clock follows frames",
                                      g_fixed_step_calls, g_fixed_fps); }
    {   /* The LCG seed is taken inside fn_03af18 (0x03af59), so the pin must be in place BEFORE the
         * game main runs, not in tick_pump (docs/107). */
        const char *sd = getenv("NIGHT_SEED");
        if (sd) g_rng_seed_override = atoi(sd);
    }
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
    if (g_act_trace) {
        static const char *why[] = {"matcher visits", "code0 mismatch", "slot consumed",
                                    "codeK mismatch", "level too high", "len != k",
                                    "face byte", "hurt target != 0x99", "rank gate",
                                    "chain MATCHED"};
        const uint32_t *c = act_trace_counts();
        SDL_Log("act-chain census (.act matcher, docs/110):");
        for (int i = 0; i < 10; i++) SDL_Log("  %-22s %u", why[i], c[i]);
        const uint32_t *tg = act_trace_targets();
        SDL_Log("  matched chain targets:");
        for (int i = 0; i < 0x100; i++) if (tg[i]) SDL_Log("    target %#04x  %u", i, tg[i]);
        SDL_Log("exp award path: adds=%u added=%u commits=%u committed=%u",
                g_exp_awards, g_exp_added, g_exp_commits, g_exp_committed);
        SDL_Log("element-event sounds: played=%u outside-pool=%u", g_sfx_plays, g_sfx_oob);
        const uint32_t *ta = act_trace_targets_all();
        SDL_Log("  targets offered by the container (one-shot inventory):");
        for (int i = 0; i < 0x100; i++) if (ta[i]) SDL_Log("    target %#04x  %u", i, ta[i]);
    }
    if (getenv("NIGHT_CENSUS")) {
        char cp[600];
        snprintf(cp, sizeof cp, "%s/census.txt", g_rig_dir ? g_rig_dir : "/tmp");
        rig_census_report(cp);
    }
    if (g_rig_dir) rig_close();
    SDL_Log("frames presented: %d", g_frames);

    if (g_tex) SDL_DestroyTexture(g_tex);
    if (g_ren) SDL_DestroyRenderer(g_ren);
    if (g_win) SDL_DestroyWindow(g_win);
    free(g_pixels);
    cd_iso_close();
    if (getenv("NIGHT_TAB532"))   /* docs/83: witness for the fn_03f6bc tick-table walk */
        fprintf(stderr, "tab53200 walk: max_cursor=%u clamped=%u limit=%u\n",
                g_tab532_walk_max, g_tab532_walk_clamped, TAB532_WALK_LIMIT);
        blit_stats_dump();
SDL_Quit();
    return 0;
}

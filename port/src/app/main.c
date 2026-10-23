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

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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
static SDL_AudioStream *g_cdda_stream;
static uint32_t  g_cdda_lba, g_cdda_sectors, g_cdda_pos;
static uint64_t  g_cdda_bytes, g_cdda_sector_reads;
static uint32_t  g_cdda_tracks;
static uint64_t  g_cdda_clock, g_cdda_freq;      /* drain-model timing */
static uint8_t   g_cdda_raw[2352];

static void cdda_play(uint32_t index, uint32_t lba, uint32_t sectors) {
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
}

static void cdda_stop(void) {
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
    }
}

/* No host stream: drain the board's ring into a scratch buffer. The game waits on the *board* (its DMA
 * transfer completing and the DSP accepting a stop), not on the host device, so the emulated DMA clock
 * has to keep advancing or fn_04185e never returns and the game logic starves. */
static void audio_pump_drain_only(void) {
    static int16_t sink[2048];
    sys_audio_set_source(g_dat_63fb4);
    uint32_t got;
    while ((got = sys_audio_fill(sink, 2048)) != 0) g_audio_drained += got * 2;
    if (getenv("NIGHT_AUDIODBG") && (g_frames % 60) == 0)
        SDL_Log("audiodbg: frame=%u no-stream drained=%llu B transfers=%u stops=%u rate=%u width=%u "
                "pending=%u pushed=%u", g_frames, (unsigned long long)g_audio_drained,
                sys_audio_transfers(), sys_audio_stops(), sys_audio_rate(), sys_audio_width(),
                sys_audio_pending(), sys_audio_pushed());
}

/* The game's sound board is only initialised if the hardware answers: fn_040f4f scans the device
 * ports for the 0xaa signature, fn_040fb3 needs its probe command (0xf2) to raise the device
 * interrupt, and fn_0411d9 needs a non-zero id. On a bare Mac none of that exists, so DAT_00063f90
 * stays 0 and fn_04185e (the sample player) is never entered. Model the board exactly as the unit
 * fixture does: a controller at 0x220 whose status port answers independently of the command port,
 * plus the interrupt the probe raises. */
static void board_cmd_hook(uint16_t port, uint8_t cmd) {
    (void)port;
    if (cmd == 0xf2) sys_isr_fire(0x0a);              /* the probe command asserts the device IRQ */
}
static void board_boot(void) {
    static const uint8_t sig[] = { 0xaa };
    sys_dev_read_seq(0x22a, sig, 1);                  /* the discovery probe's signature read */
    sys_dev_status_set(0x22e, 0x80);                  /* ready for reads  */
    sys_dev_status_set(0x22c, 0x00);                  /* ready for writes */
    sys_isr_bind(0x41439, fn_041439);                 /* the game's own vector-0x0a handler */
    sys_dev_cmd_hook(board_cmd_hook);
    g_dat_53334 = 0x220;      /* the shipped image leaves 0x210 (board disabled); opt in with NIGHT_BOARD=1 */
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
struct pt_evt { long t_ms; int kind; uint8_t sc; int make; char name[40]; uint32_t seq_at; };
static void shot_named(const char *event);
static void key_apply(uint8_t sc, int make);
enum { PT_KEY = 0, PT_SHOT = 1, PT_WAIT = 2 };
static struct pt_evt *g_pt;
static int g_pt_n, g_pt_i;
static const char *g_shots_dir;
static FILE *g_manifest;
static int g_shot_seq;
static int g_shot_limit = 600;
static int g_shot_scene = 1, g_shot_res = 1, g_shot_fli = 1;
/* Opening a host audio device can block outright on a machine whose CoreAudio devices are broken (the
 * open call never returns, not merely fails), which must not stop a scripted playtest run. */
static int g_no_audio;
static uint32_t g_pt_res_seq;
static Uint64 g_pt_last_scene, g_pt_last_fli;
static uint64_t g_pt_hash;
static uint8_t g_pt_sample[2048];

static long pt_now_ms(void) {
    return (long)((SDL_GetPerformanceCounter() - g_start_time) * 1000ull / g_perf_freq);
}

/* the DOS scan code the game's key state consults for a script token */
static uint8_t pt_sc(const char *name) {
    static const struct { const char *n; uint8_t sc; } tbl[] = {
        { "enter", 0x1c }, { "return", 0x1c }, { "space", 0x39 }, { "esc", 0x01 },
        { "tab", 0x0f }, { "up", 0x48 }, { "down", 0x50 }, { "left", 0x4b }, { "right", 0x4d },
        { "ctrl", 0x1d }, { "shift", 0x2a }, { "backspace", 0x0e },
    };
    for (unsigned k = 0; k < sizeof tbl / sizeof tbl[0]; k++)
        if (!strcasecmp(name, tbl[k].n)) return tbl[k].sc;
    if (name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) return (uint8_t)strtoul(name, NULL, 16);
    if (strlen(name) == 2 && isxdigit((unsigned char)name[0]) && isxdigit((unsigned char)name[1]))
        return (uint8_t)strtoul(name, NULL, 16);
    SDL_Scancode c = SDL_GetScancodeFromName(name);
    if (c >= SDL_SCANCODE_A && c <= SDL_SCANCODE_Z) return (uint8_t)(0x1e + (c - SDL_SCANCODE_A));
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
        long t; char verb[64], arg[64], updown[64];
        int n = sscanf(line, "%ld %63s %63s %63s", &t, verb, arg, updown);
        if (n < 2) continue;
        struct pt_evt *e = &g_pt[g_pt_n];
        e->t_ms = t; e->kind = PT_KEY; e->make = 1; e->name[0] = 0; e->sc = 0; e->seq_at = 0;
        if (!strcasecmp(verb, "shot")) {
            e->kind = PT_SHOT; snprintf(e->name, sizeof e->name, "%s", arg);
        } else if (!strcasecmp(verb, "wait") && n >= 3 && !strcasecmp(arg, "res")) {
            e->kind = PT_WAIT; snprintf(e->name, sizeof e->name, "%s", updown);
            e->seq_at = 0;   /* set when the wait becomes current, see pt_drive */
            if (e->name[0] == 0) { SDL_Log("script: wait res needs a name (line ignored)"); continue; }
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
static char g_reslog[256][24];
static int  g_reslog_n, g_reslog_cursor;
static char g_res_match[24];
static long g_pt_done_ms = -1;                       /* when the last scripted event fired, -1 = not yet */
static void reslog_add(const char *name) {
    if (g_reslog_n < (int)(sizeof g_reslog / sizeof g_reslog[0])) {
        snprintf(g_reslog[g_reslog_n], sizeof g_reslog[0], "%s", name);
        g_reslog_n++;
    }
}

static void pt_drive(void) {
    if (!g_pt || !g_pt_n) return;
    long t = pt_now_ms();
    while (g_pt_i < g_pt_n) {
        struct pt_evt *e = &g_pt[g_pt_i];
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
static struct { char name[72]; Uint64 due; } g_pt_pending[PT_MAX_PENDING];
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
    g_pt_pending_n++;
}

static void shot_flush_pending(void) {
    Uint64 now = SDL_GetPerformanceCounter();
    for (int i = 0; i < g_pt_pending_n; i++) {
        if (now < g_pt_pending[i].due) continue;
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
static void shots_resource_edge(void) {
    if (!g_shots_dir || !g_shot_res || g_res_edge_busy) return;
    if (g_res_trace_seq == g_pt_res_seq) return;
    g_res_edge_busy = 1;
    g_pt_res_seq = g_res_trace_seq;
    g_pt_hash = 0;
    shot_named(g_res_trace);
    shot_queue(g_res_trace);
    g_pt_last_scene = SDL_GetPerformanceCounter();
    shot_flush_pending();
    g_res_edge_busy = 0;
}

static void shots_poll(void) {
    if (!g_shots_dir) return;
    shot_flush_pending();
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
        case SDL_SCANCODE_ESCAPE:  sc = 0x01; g_quit = 1; break;
        case SDL_SCANCODE_SPACE:   sc = 0x39; break;
        case SDL_SCANCODE_LCTRL:   sc = 0x1d; break;
        case SDL_SCANCODE_LSHIFT:  sc = 0x2a; break;
        default: break;
        }
        if (!sc) continue;
        key_apply(sc, ev.key.down);
    }
}

/* the game-side half of input handling: SDL key events and the playtest script both land here, so a
 * scripted run drives exactly the bytes an interactive run does */
static void key_apply(uint8_t sc, int make) {
        /* The four arrow keys drive the game's dedicated "held key" bytes, and each screen reads a
         * different pair, which is how the pairs' meaning is pinned from the consumers (all ported, with
         * original addresses): fn_038cde / the help scroll use DAT_00063ed3 (0x00038d7b, up) and
         * DAT_00063ed5 (0x00038df0, down); fn_039c03 (the setup menu) walks its entry index with
         * DAT_00063ed0 (0x00039d2a) and DAT_00063ed8 (0x00039d86) and toggles the selected entry's value
         * with DAT_00063ed3 / DAT_00063ed5 (0x00039de2) -- so Left/Right move between entries and
         * Up/Down change the value, and the arrows are the ed0/ed8 pair. The original set these in its
         * keyboard handler (not in the pump fn_0347e3, which only walks the scan-code slots), so the host
         * writes them directly. docs/60. */
        switch (sc) {
        case 0x48: g_dat_63ed3 = (uint8_t)make; break;      /* up */
        case 0x50: g_dat_63ed5 = (uint8_t)make; break;      /* down */
        case 0x4b: g_dat_63ed0 = (uint8_t)make; break;      /* left */
        case 0x4d: g_dat_63ed8 = (uint8_t)make; break;      /* right */
        default: break;
        }
        /* The game's own key bytes (the DS bytes its keyboard pump used to set; the menu in fn_03a224
         * and every key wait read these): 63ea4 = confirm, 63ed0/63ed8 = the held select keys,
         * 63e89 = abort, 63ec1 = a plain key press. The arrows are deliberately NOT "any key": fn_039c03
         * leaves the menu with m = 0x64 as soon as DAT_00063eb2 / DAT_00063ebe are set (0x00039ce6-d25),
         * so feeding them from an arrow press would make the menu impossible to navigate (docs/60). */
        uint8_t *byte = (sc == 0x1c || sc == 0x39) ? &g_dat_63ea4      /* enter / space: confirm */
                    : (sc == 0x01) ? &g_dat_63e89                      /* esc: abort */
                    : (sc == 0x48 || sc == 0x50 || sc == 0x4b || sc == 0x4d) ? (uint8_t *)0
                    : &g_dat_63ec1;
        if (byte != (uint8_t *)0) *byte = (uint8_t)make;
        if (sc != 0x48 && sc != 0x50 && sc != 0x4b && sc != 0x4d) {
            /* The "any key" bytes follow the key: set on make, cleared on break. If they latched, the
             * setup menu (fn_039c03) would leave on its very first frame -- its entry check spins until
             * DAT_00063e89 / 0x63eb2 / 0x63ebe are clear and then exits with m = 0x64 (0x00039ce6-d25) --
             * and every "wait for a key" loop would fall through. */
            g_dat_63eb2 = (uint8_t)make; g_dat_63ebe = (uint8_t)make;
        }
        if (make) {
            g_dat_62c58[sc] = 1;                           /* scan-code slot, as fn_0343d1 arms it */
        } else {
            g_dat_62c58[sc] = 0;
        }
}

static void input_pump(void) {
    autokey();                                                 /* the scripted headless key press */
    pt_drive();                                                /* the playtest script's key timeline */
    shots_resource_edge();                                     /* resource names announced inside a nested
                                                                * screen are only visible from a spin point */
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
            (int)g_dat_53c68, n, (int)g_dat_639dc, (int)g_dat_639e0, (void *)g_dat_53c7c);
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
                k, (void *)e, ty, sty, x, y, st, face, e == g_dat_53c7c,
                *(uint32_t *)(e + 0x127), *(uint32_t *)(e + 0x12b), *(uint32_t *)(e + 0x12f));
        e = *(uint8_t **)(e + 0x13f);
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
                "pit=%u Hz retrace=%lu audio=%uHz pushed=%u pend=%u xfer=%u stops=%u cdda=%lluB tracks=%u pos=%u snden=%u devinit=%u plays=%u real=%lu null=%lu cdstops=%u sfx=%u keyb_ed0_ed3_ed5_ed8=%u/%u/%u/%u",
                g_pump_calls, g_timer_calls, g_frames, g_dat_63e24, g_dat_63e40, g_dat_63e3c,
                sys_pit_hz(0), (unsigned long)g_vga_retrace_waits,
                sys_audio_rate(), sys_audio_pushed(), sys_audio_pending(),
                sys_audio_transfers(), sys_audio_stops(),
                (unsigned long long)g_cdda_bytes, (unsigned)g_cdda_tracks, (unsigned)g_cdda_pos,
                (unsigned)g_dat_53338, (unsigned)g_dat_63f90, (unsigned)g_play_calls, g_play_real, g_play_null,
                (unsigned)sys_cd_audio_stop_calls(), (unsigned)g_dat_51a10,
                (unsigned)g_dat_63ed0, (unsigned)g_dat_63ed3, (unsigned)g_dat_63ed5, (unsigned)g_dat_63ed8);
        /* Host debug warp for screens a scripted route cannot reach in the time a run lasts: the level-up
     * (fn_038f3d, variant 1 -- the game itself calls it at 0x0003b54c right after g_dat_63370 += 1 on stage
     * completion, guarded by pl[0x30] != 1) and the status (variant 0) / help (fn_038cde, the pause menu's
     * entries 0 and 1 at 0x0003d2ac / 0x0003d2c1) screens. The warp calls the game's own routine, so the
     * asset loads, the compositing and the key handling under test are the shipped ones; only the moment of
     * entering them is host-chosen (same convention as --warp-stage, docs/52).
     * NIGHT_SHOWSCREEN=levelup|status|help, NIGHT_SHOWSCREEN_AT=<frame>. */
    if (getenv("NIGHT_MENUBG")) g_menu_trace = 1;
    if (getenv("NIGHT_HUDDBG")) g_hud_trace = 1;
        { static int s_shown;                                  /* the debug warp for the menu screens */
          const char *what = getenv("NIGHT_SHOWSCREEN");
          long at = getenv("NIGHT_SHOWSCREEN_AT") ? atol(getenv("NIGHT_SHOWSCREEN_AT")) : 4000;
          if (what && !s_shown && (long)g_frames >= at) {
              s_shown = 1;
              SDL_Log("showscreen warp: %s at frame %u", what, g_frames);
              if (!strcmp(what, "levelup"))      fn_038f3d(1);
              else if (!strcmp(what, "status"))  fn_038f3d(0);
              else if (!strcmp(what, "help"))    fn_038cde();
              else SDL_Log("showscreen warp: unknown screen '%s'", what);
          } }
    if (getenv("NIGHT_ENTDBG")) ent_dump();
        if (getenv("NIGHT_BLITSTATS")) blit_stats_dump();
        if (getenv("NIGHT_HEAPDUMP")) { static int s_hd; if (!s_hd && g_dat_63360 >= 900 && !s_hd++) {
            FILE *fp = fopen("/tmp/heap.bin", "wb");
            if (fp) { fwrite((const void *)g_heap_base, 1, heap_arena_cap_get(), fp); fclose(fp); }
            SDL_Log("heap dump: base=%p cap=%#x", (void *)g_heap_base, heap_arena_cap_get());
        } }
        g_diag_next = now + g_perf_freq;
    }
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
    while (calls--) { fn_03ee88(); g_timer_calls++; }

    if (now >= g_present_next) {
        present();
        g_frames++;
        for (unsigned k = 0; k < sizeof g_dump_frames / sizeof g_dump_frames[0]; k++)
            if (g_frames == g_dump_frames[k]) dump_frame(g_frames);
        shots_poll();                                       /* named-event playtest screenshots */
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
        else if (!strcmp(argv[i], "--autokey")) g_autokey = 1;
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) pt_load(argv[++i]);
        else if (!strcmp(argv[i], "--no-audio")) g_no_audio = 1;
        else if (!strcmp(argv[i], "--shots") && i + 1 < argc) g_shots_dir = argv[++i];
        else if (!strcmp(argv[i], "--shot-limit") && i + 1 < argc) g_shot_limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--shot-delay") && i + 1 < argc) g_shot_delay_ms = (Uint64)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--warp-stage") && i + 1 < argc) g_warp_stage = atoi(argv[++i]);
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
    if (getenv("NIGHT_BOARD")) board_boot();   /* else the shipped default (DAT_53334 = 0x210) is kept */
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
                    *(const uint32_t *)(e + 0x0d), *(const uint32_t *)(e + 0x11));
        }
    }
    sys_set_input_pump(input_pump);
    sys_set_tick_pump(tick_pump);

    g_start_time = SDL_GetPerformanceCounter();
    g_tick_next = g_start_time;
    g_present_next = g_start_time;
    g_diag_next = g_tick_next + g_perf_freq;

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
    free(g_pixels);
    cd_iso_close();
        blit_stats_dump();
SDL_Quit();
    return 0;
}

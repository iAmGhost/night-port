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

#include <math.h>
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

static void audio_boot(void) {
    if (!SDL_Init(SDL_INIT_AUDIO)) { SDL_Log("audio unavailable: %s", SDL_GetError()); return; }
    SDL_AudioSpec dst; SDL_zero(dst);
    dst.format = SDL_AUDIO_S16; dst.channels = 2; dst.freq = 48000;
    g_adev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &dst);
    if (!g_adev) { SDL_Log("audio device: %s", SDL_GetError()); return; }
    SDL_AudioSpec src; SDL_zero(src);
    src.format = SDL_AUDIO_S16; src.channels = 1; src.freq = 22050;
    g_astream_rate = 22050;
    g_astream = SDL_CreateAudioStream(&src, &dst);
    SDL_ResumeAudioDevice(g_adev);                            /* SDL3 devices open paused */
    SDL_Log("audio: playing at 48000 Hz (board samples start at %u Hz)", g_astream_rate);
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
    if (!g_adev || !g_astream) return;
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
        if (!SDL_PutAudioStreamData(g_astream, buf, (int)(got * 2))) break;
    }
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
        int make = ev.key.down;
        switch (sc) {
        case 0x48: g_dat_63ed3 = (uint8_t)make; break;      /* up held */
        case 0x50: g_dat_63ed5 = (uint8_t)make; break;      /* down held */
        default: break;
        }
        /* The game's own key bytes (the DS bytes its keyboard pump used to set; the menu in fn_03a224
         * and every key wait read these): 63ea4 = confirm, 63ed0/63ed8 = the held select keys,
         * 63e89 = abort, 63ec1 = a plain key press. */
        uint8_t *byte = (sc == 0x1c || sc == 0x39) ? &g_dat_63ea4      /* enter / space: confirm */
                    : (sc == 0x01) ? &g_dat_63e89                      /* esc: abort */
                    : (sc == 0x48 || sc == 0x50 || sc == 0x4b || sc == 0x4d) ? &g_dat_63ed0
                    : &g_dat_63ec1;
        *byte = (uint8_t)make;
        if (make) {
            g_dat_63eb2 = 1; g_dat_63ebe = 1;              /* the "any key" bytes */
            g_dat_62c58[sc] = 1;                           /* scan-code slot, as fn_0343d1 arms it */
        } else {
            g_dat_62c58[sc] = 0;
        }
    }
}

static void input_pump(void) {
    autokey();                                                 /* the scripted headless key press */
    sdl_events();
    fn_0347e3();                                           /* the game's keyboard pump */
}

/* ---- the tick pump: real time -> the installed timer handler ---------- */
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
    }
    if (now >= g_diag_next) {                      /* progress diagnostics: is the clock moving? */
        SDL_Log("pump: calls=%lu timer_calls=%lu frames=%u tick=%u DAT_63e40=%u DAT_63e3c=%u "
                "pit=%u Hz audio=%uHz pushed=%u pend=%u xfer=%u stops=%u snden=%u devinit=%u plays=%u",
                g_pump_calls, g_timer_calls, g_frames, g_dat_63e24, g_dat_63e40, g_dat_63e3c,
                sys_pit_hz(0), sys_audio_rate(), sys_audio_pushed(), sys_audio_pending(),
                sys_audio_transfers(), sys_audio_stops(),
                (unsigned)g_dat_53338, (unsigned)g_dat_63f90, (unsigned)g_play_calls);
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
        else if (!strcmp(argv[i], "--help")) {
            printf("usage: night [--res-dir DIR] [--frames N] [--dump DIR]\n");
            return 0;
        }
    }
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--resnames")) g_resnames = 1;
    if (getenv("NIGHT_RES_DIR") && *getenv("NIGHT_RES_DIR")) res_dir = getenv("NIGHT_RES_DIR");
    char cue[1024];
    snprintf(cue, sizeof cue, "%s/TG_NIGHT.CUE", res_dir);

    if (getenv("NIGHT_PAL")) {
        if (!strcmp(getenv("NIGHT_PAL"), "staging")) g_pal_src = 1;
        else if (!strcmp(getenv("NIGHT_PAL"), "cache")) g_pal_src = 2;
        else if (!strcmp(getenv("NIGHT_PAL"), "cacheraw")) g_pal_src = 3;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (getenv("NIGHT_BOARD")) board_boot();   /* else the shipped default (DAT_53334 = 0x210) is kept */
    if (getenv("NIGHT_SOUND")) g_dat_53338 = 1;  /* the title menu's SOUND toggle (fn_039c03) is off by
                                                  * default in the shipped image; force it on for tests */
    audio_boot();
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

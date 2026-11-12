/* main_wasm.c -- the browser (WebAssembly) host twin of src/app/main.c.  docs/457.
 *
 *   cd wasm && make serve      ->  http://127.0.0.1:8091/  (NIGHT in the tab)
 *
 * ARCHITECTURE: the game layer is the same code the desktop and Pocket hosts run (src/f, src/sys,
 * src/app/keymap.c compiled in place -- one copy, no fork), and the cooperative model is the same one
 * docs/18 documents: fn_03af18 owns the call stack, and the two things the original got from hardware
 * are delivered at the points where the original waited -- the tick pump at the PIT rate the game
 * programmed, and input drained into the DS key bytes the keyboard pump owns. What differs is only the
 * host's side of the same three seams:
 *
 *   frame   the composed RGBA goes to a <canvas> with putImageData instead of to an SDL renderer, and
 *           the host YIELDS: the game's whole stack is unwound (-sASYNCIFY) so the browser gets back
 *           its event loop, paints, and calls us again at the next requestAnimationFrame. There is no
 *           main loop here -- main() runs the entire game and suspends inside it ~60 times a second.
 *   input   runtime/shell.js pushes (make, break) scancode pairs into a ring in wasm memory and never
 *           calls a wasm function to do it. The block layout is wasm_block.h, and it is a block and not calls
 *           for one reason: while the game is yielded its stack is mid-unwind, and a second JS -> wasm
 *           entry in that window (a keydown handler, an AudioContext callback) would write its own
 *           frames into the same unwind bookkeeping and corrupt the sleeping stack. The only JS -> wasm
 *           entry in the whole browser build is the one that resumes a yield.
 *   audio   no SDL, and no device callback either: the host pushes the sound board's DMA ring and the
 *           CD-DA sectors into two PCM rings in wasm memory, and runtime/shell.js' ScriptProcessorNode
 *           pulls them with pure JS. Same-thread, single producer / single consumer, and again no entry.
 *   disc    the CD-DA bytes come from HTTP Range reads (wasm_disc.c) instead of trackNN.bin, so hearing
 *           one song costs the ~150 KB window it is playing, not the 604 MB disc.
 *
 * WHY NOT SDL IN THE BROWSER: emscripten's SDL3 port works, and its audio backend enters wasm from a
 * JS audio callback and its video driver enters wasm from canvas key handlers -- both exactly the
 * reentrant entry Asyncify forbids. Dropping SDL removes that class of failure and 1.5 MB of binary,
 * at the cost of ~900 lines of host, which is the trade the Pocket host (pocket/src/night/main_of.c)
 * already made for the same reason. */

#include "f/f.h"
#include "sys/sys.h"
#include "cd/cd_iso.h"
#include "cd/cd_audit.h"
#include "app/keymap.h"

#include "wasm_block.h"
#include "wasm_disc.h"

#include <emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- the shared block (wasm_block.h) ---------------------------------------------
 * JS gets its address once, at boot, from nk_block_addr() -- a call made before the game
 * main is entered, while nothing is unwound. After that JS only reads and writes bytes here. */
static uint32_t g_block[NK_KEY_WORDS + NK_KEYQ_SLOTS];

/* ---- the two PCM rings -------------------------------------------------------------
 * 16384 stereo frames = 372 ms at 44.1 kHz: enough to ride out a fetch yield, short enough to keep the
 * heard stream under half a second behind the game. */
#define NK_RING_FRAMES (1u << 14)
/* The rate a ring holds when nothing has said otherwise: 44.1 kHz, which is what a Red Book track is.
 * The sound board's ring follows the board instead (ring_rate_sync), and the page resamples whichever it
 * reads to its AudioContext rate. */
#define NK_DEFAULT_RATE 44100u
#define NK_CDDA_RATE    44100u
/* Half sys.c's board ring (AUDIO_RING, src/sys/sys.c:153): the point past which leaving the sound board's
 * bytes in place would cost bytes the board has nowhere to put them, so the host drops them instead. */
#define NK_BOARD_BACKLOG_BYTES 65536u
static uint32_t g_ring_words[2][NK_RING_WORDS];
static int16_t  g_ring_pcm[2][NK_RING_FRAMES * 2];
/* Per-ring rate marks: [0] = count, then (absolute frame, rate) pairs. See wasm_block.h. */
static uint32_t g_ring_marks[2][NK_RING_MARK_STRIDE];

EMSCRIPTEN_KEEPALIVE uint32_t nk_block_addr(void) { return (uint32_t)(uintptr_t)g_block; }
EMSCRIPTEN_KEEPALIVE uint32_t nk_ring_marks_addr(int which) {
    if ((unsigned)which >= 2u) return 0;
    return (uint32_t)(uintptr_t)g_ring_marks[which];
}
EMSCRIPTEN_KEEPALIVE uint32_t nk_ring_addr(int which) {
    if ((unsigned)which >= 2u) return 0;
    return (uint32_t)(uintptr_t)g_ring_words[which];
}
EMSCRIPTEN_KEEPALIVE uint32_t nk_ring_frames(void) { return NK_RING_FRAMES; }

/* ---- the JS side (runtime/shell.js owns the NightWasm object) -----------------------
 * NOTE on EM_JS arguments: this emscripten emits `f(a,b){...}` from the C signature, so the body
 * refers to the parameters by their plain names (NOT $a). Pinned in the Makefile by an EMSDK version. */
EM_JS(void, nk_js_present, (const uint32_t *rgba, int w, int h), {
    window.NightWasm.present(rgba, w, h);
});
EM_ASYNC_JS(void, nk_yield_frame, (void), { await window.NightWasm.nextFrame(); });

/* ---- monotonic time -------------------------------------------------------------- */
static uint64_t nk_now_us(void) {
    struct timespec ts;
#if defined(CLOCK_MONOTONIC)
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
#endif
    return (uint64_t)clock() * 1000000ull / (uint64_t)CLOCKS_PER_SEC;
}

/* ---- the game's flat-model buffers the host composes from ------------------------- */
static uint8_t g_node_pool[0x53800];        /* the resource node pool (docs/15 boot recipe) */

static int      g_quit;
static int      g_exit_state;               /* 0 running, 1 asked, 2 the game main returned */
static uint64_t g_exit_deadline_us;
static uint32_t g_frames;
static uint32_t g_timer_calls;
static uint64_t g_tick_next_us, g_present_next_us, g_diag_next_us, g_audio_next_us, g_last_pump_us;
/* Where the time goes, per 5 s window: a browser build has no separate frame thread to blame, so these
 * four numbers are the whole performance story -- how often the pump is called, how long one game tick
 * costs, how long a composition costs, and how long the yield to the browser holds the game. */
static uint32_t g_pumps, g_tick_calls, g_frames_win;
static uint64_t g_gap_us, g_gap_max_us, g_step_us, g_present_us, g_yield_us, g_yield_max_us;
static uint64_t g_start_us, g_last_log_us;
/* The diagnostics cadence (`--diagms`, default 5 s). Measuring a ~7 s animation through a 5 s log line is
 * measuring nothing, so a span run drops it to 1000 ms and reads the wipe's first and last changing frame. */
static uint64_t g_diag_us = 5000000ull;

/* ---- fatal / terminate ----------------------------------------------------------- */
static void on_fatal(const char *msg) {
    printf("port fatal: %s\n", msg);
    g_quit = 1; g_exit_state = 1;
    if (!g_exit_deadline_us) g_exit_deadline_us = nk_now_us() + 3000000ull;
}
static uint32_t on_terminate(uint32_t a) {
    printf("terminate requested (DAT_5336c/70/74 callback, arg %u)\n", a);
    g_quit = 1; g_exit_state = 1;
    if (!g_exit_deadline_us) g_exit_deadline_us = nk_now_us() + 3000000ull;
    return 0;
}

/* Leaving without longjmp. src/app/main.c unwinds the game main with setjmp/longjmp; longjmp across a
 * stack that asyncify has already unwound and rewound is a documented unsupported combination, so the
 * browser host never does it. The game is asked to stop and given 3 s of pumped frames to do it; after
 * that the runtime ends the program. The visible behaviour is the same: the picture stops. */
static void nk_exit_check(void) {
    if (!g_exit_deadline_us || nk_now_us() < g_exit_deadline_us) return;
    printf("exit: the game main did not return within 3 s -- ending the program\n");
    g_exit_state = 2;
    g_block[NK_ST_EXIT] = 2;
    emscripten_force_exit(0);
}

/* ---- presentation ---------------------------------------------------------------- */
static int g_vid_w, g_vid_h;
static uint32_t *g_pixels;
static uint8_t g_palette8[768];

/* The DAC holds 6-bit values (the game never writes above 63), so they are scaled the way a VGA DAC
 * scales them into 8-bit RGB -- both facts measured, docs/47 and docs/86. */
static void palette_rgba(void) {
    sys_vga_palette(g_palette8);
    if (sys_vga_dac_max() <= 63)
        for (int i = 0; i < 768; i++) g_palette8[i] = (uint8_t)((g_palette8[i] << 2) | (g_palette8[i] >> 4));
}

static int ensure_mode(int w, int h) {
    if (w == g_vid_w && h == g_vid_h && g_pixels) return 0;
    free(g_pixels);
    g_pixels = malloc((size_t)w * (size_t)h * 4u);
    if (!g_pixels) return -1;
    printf("display mode %dx%d\n", w, h);
    g_vid_w = w; g_vid_h = h;
    g_block[NK_ST_VID_W] = (uint32_t)w;
    g_block[NK_ST_VID_H] = (uint32_t)h;
    return 0;
}

/* The room path composites a 320x200 frame and flips those 64000 bytes to the framebuffer start
 * (fn_038098); the slide path fills the whole 640x480 arena (fn_03f462). Present whichever was written
 * last -- the same decision src/app/main.c makes. */
static void present(void) {
    int vw = g_view_320x200 ? 320 : sys_video_w(), vh = g_view_320x200 ? 200 : sys_video_h();
    if (ensure_mode(vw, vh) != 0) return;
    palette_rgba();
    const uint8_t *src = g_video_ram;
    /* ImageData bytes are R,G,B,A in memory, so on this little-endian machine the red byte is the LOW
     * byte of the uint32 -- the opposite of the SDL renderer's RGBA8888 masks the desktop host uses. */
    for (int i = 0; i < vw * vh; i++) {
        uint8_t idx = src[i];
        g_pixels[i] = (uint32_t)g_palette8[idx * 3]
                    | ((uint32_t)g_palette8[idx * 3 + 1] << 8)
                    | ((uint32_t)g_palette8[idx * 3 + 2] << 16)
                    | 0xff000000u;
    }
    nk_js_present(g_pixels, vw, vh);
}

/* ---- the yield --------------------------------------------------------------------
 * The frame boundary is where the browser wants the thread back. Yielding there is not enough on its
 * own: a screen that runs entirely inside one game step (an intro card, a .fli) can hold the stack for
 * far longer than a frame, so the pump also yields on a wall-clock deadline. Both are the same call and
 * the same rule: never enter wasm from JS while the stack is unwound, and never let the game run longer
 * than the watchdog without giving the browser a turn. */
#define NK_FRAME_US       16667ull      /* present at ~60 Hz */
#define NK_YIELD_MAX_US   40000ull      /* yield even when nothing presented, every 40 ms */
static uint64_t g_yield_next_us;

static void nk_yield(const char *why) {
    uint64_t now = nk_now_us();
    if (now < g_yield_next_us) return;
    g_yield_next_us = now + NK_FRAME_US;
    nk_yield_frame();
    (void)why;
}

/* ---- input ------------------------------------------------------------------------
 * night.js fills the ring; the pump drains it into the game's key bytes through keymap.c (docs/91),
 * which is the same map the fixture suite asserts against. */
static uint32_t g_keys = 0;

static void key_drain(void) {
    uint32_t head = g_block[NK_KEY_HEAD], tail = g_block[NK_KEY_TAIL];
    uint32_t size = g_block[NK_KEY_SIZE], mask = size - 1u;
    uint32_t *slots = (uint32_t *)&g_block[g_block[NK_KEY_SLOT_BYTES] / 4u];
    while (tail != head) {
        uint32_t e = slots[tail & mask];
        night_key_apply((uint8_t)(e & 0xffu), (e >> 8) & 1u);
        tail++;
        g_keys++;
    }
    g_block[NK_KEY_TAIL] = tail;
    g_block[NK_ST_KEYS] = g_keys;
}

static void input_pump(void) {
    key_drain();
    fn_0347e3();                        /* the game's keyboard pump */
}

/* ---- audio ------------------------------------------------------------------------
 * Two rings (the sound board's DMA ring, and CD-DA), each a free-running index pair the page consumes
 * in its audio callback. The producer resamples to the page's AudioContext rate; CDDA at 44.1 kHz
 * usually needs no resampling at all. */
static uint8_t g_cdda_sector[2352] __attribute__((aligned(4)));

/* Each ring carries frames at ONE rate: the rate of whoever wrote them. The sound board's DMA ring runs at
 * the rate the last DSP 0x40 programmed (11 or 22 kHz), CD-DA at 44.1 kHz because that is what a Red Book
 * track is. The page resamples ring rate -> AudioContext rate as it mixes, and holds its fractional read
 * position in its own reader state: the ring IS the carry buffer, so the place in the stream survives an
 * underrun, a board-rate change and a track switch with nothing to reconcile on this side. */
static uint32_t ring_used(int w)  { return g_ring_words[w][NK_RING_HEAD] - g_ring_words[w][NK_RING_TAIL]; }
static uint32_t ring_room(int w)  { return g_ring_words[w][NK_RING_CAP] - ring_used(w); }
static uint32_t ring_target(int w, uint32_t ms) { return g_ring_words[w][NK_RING_RATE] * ms / 1000u; }

/* The live entries are the ordinals [ord0, ord0+n). Slot = ordinal % CAP, so trimming the front moves
 * nothing. The base entry is always there: the page needs the rate of the frames it starts on. */
static void ring_marks_seed(int w, uint32_t frame, uint32_t rate) {
    uint32_t *M = g_ring_marks[w];
    uint32_t o = M[NK_RING_MARK_ORD] + M[NK_RING_MARK_N];      /* the ordinal space never resets, so a reader
                                                                * still looking at the old entries cannot read a
                                                                * new one as if it were an old one */
    M[NK_RING_MARK_N] = 1;
    M[NK_RING_MARK_ORD] = o;
    M[NK_RING_MARK_ENTRY + (o % NK_RING_MARK_CAP) * NK_RING_MARK_EWORDS] = frame;
    M[NK_RING_MARK_ENTRY + (o % NK_RING_MARK_CAP) * NK_RING_MARK_EWORDS + 1] = rate;
}

static void ring_flush(int w) {
    uint32_t h = g_ring_words[w][NK_RING_HEAD];
    g_ring_words[w][NK_RING_TAIL] = h;      /* drop what the page has not taken yet (SDL_FlushAudioStream's job) */
    ring_marks_seed(w, h, g_ring_words[w][NK_RING_RATE]);   /* and the rate changes that came with them */
    g_ring_words[w][NK_RING_MARK_GEN]++;
}

/* Record "absolute frame F onwards is at rate R", in increasing frame order, after dropping the entries the
 * page has certainly passed: the reader's read position is at or behind its own tail plus the filter window,
 * so an entry at or below the tail has been applied. Overflow past the cap is the loud path; trimming should
 * keep it from ever happening. */
static void ring_mark(int w, uint32_t frame, uint32_t rate) {
    uint32_t *M = g_ring_marks[w];
    uint32_t n = M[NK_RING_MARK_N], ord0 = M[NK_RING_MARK_ORD];
    uint32_t tail = g_ring_words[w][NK_RING_TAIL];
    while (n > 1 && M[NK_RING_MARK_ENTRY + (ord0 % NK_RING_MARK_CAP) * NK_RING_MARK_EWORDS] <= tail) {
        ord0++; n--;                                  /* the page has read past this change */
    }
    if (n >= NK_RING_MARK_CAP) {
        printf("audio: %u rate marks on ring %d -- flushing %u frames queued\n", n, w, ring_used(w));
        ring_flush(w);
        g_ring_words[w][NK_RING_RATE] = rate;
        return;
    }
    uint32_t o = ord0 + n;
    M[NK_RING_MARK_ENTRY + (o % NK_RING_MARK_CAP) * NK_RING_MARK_EWORDS] = frame;
    M[NK_RING_MARK_ENTRY + (o % NK_RING_MARK_CAP) * NK_RING_MARK_EWORDS + 1] = rate;
    M[NK_RING_MARK_N] = n + 1;
    M[NK_RING_MARK_ORD] = ord0;
    g_ring_words[w][NK_RING_RATE] = rate;
}

/* The board's rate is programmed per DMA transfer, and the game uses 10000, 10989, 11111 and 22050 Hz in
 * a single scene. A change is recorded as a mark at the frame where the new rate's bytes begin, so the
 * bytes already queued keep the rate they were written at -- the desktop host gets the same property from
 * SDL_SetAudioStreamFormat, which keeps the queued data under the old spec. Flushing here used to throw
 * away 1621 frames (37 ms) in a 34 s run. CD-DA's rate never changes. */
static uint32_t g_sfx_flushes, g_sfx_chop_bytes, g_sfx_marks;
/* Bytes still queued on the board, and frames still queued in the page, at the instant the game stopped a
 * sound. See audio_pump. */
static uint32_t g_stop_n, g_stop_board, g_stop_page, g_stop_max_board, g_stop_max_page;

static void ring_rate_sync(void) {
    uint32_t rate = sys_audio_rate();
    if (!rate) rate = NK_DEFAULT_RATE;                  /* nothing programmed yet: the rate CDDA is at */
    uint32_t *I = g_ring_words[NK_RING_SFX];
    if (I[NK_RING_RATE] != rate) {
        g_sfx_marks++;
        ring_mark(NK_RING_SFX, I[NK_RING_HEAD], rate);      /* the new rate's bytes begin at HEAD */
    }
    g_ring_words[NK_RING_CDDA][NK_RING_RATE] = NK_CDDA_RATE;
}

/* `nframes` frames of `schans` channels into ring `w` -- a copy with mono expanded to stereo, since the
 * ring is already at the producer's rate. Returns the frames written. */
static uint32_t ring_push(int w, const int16_t *src, uint32_t nframes, int schans, uint32_t budget) {
    uint32_t room = ring_room(w);
    if (budget < room) room = budget;
    if (!room || !nframes) return 0;
    uint32_t *I = g_ring_words[w];
    int16_t *pcm = g_ring_pcm[w];
    uint32_t mask = I[NK_RING_MASK], head = I[NK_RING_HEAD], pushed = 0;
    while (pushed < room && pushed < nframes) {
        int16_t l = src[pushed * schans];
        int16_t r = (schans > 1) ? src[pushed * schans + 1] : l;
        uint32_t p = (head + pushed) & mask;
        pcm[p * 2] = l; pcm[p * 2 + 1] = r;
        pushed++;
    }
    I[NK_RING_HEAD] = head + pushed;
    return pushed;
}

static uint32_t g_sfx_bytes, g_sfx_dropped, g_sfx_deferred, g_cdda_bytes, g_cdda_tracks;

static void audio_pump(void) {
    ring_rate_sync();
    /* What the game leaves behind when it stops a sound. DSP 0xd0/0xd1 only disarms the DMA channel
     * (src/sys/sys.c:250) -- the bytes already in the board's ring, and the ones the page has already taken
     * into its own, both play out in full, and the next sound queues up behind them. A real Sound Blaster
     * abandons the transfer instead (docs/04-sound.md §6: single-voice, "stops previous on new play"), so
     * this is the number behind "new sounds wait for the old one to finish". */
    static uint32_t last_stops;
    uint32_t stops = sys_audio_stops();
    if (stops > last_stops) {
        uint32_t b = sys_audio_pending(), p = ring_used(NK_RING_SFX);
        g_stop_board += b; g_stop_page += p; g_stop_n++;
        if (b > g_stop_max_board) g_stop_max_board = b;
        if (p > g_stop_max_page) g_stop_max_page = p;
        last_stops = stops;
        /* The card dropped the stopped sound's bytes; the page has to drop the ones it already took, or the
         * sound keeps playing from the queue and the stop changes nothing you can hear. */
        if (p) ring_flush(NK_RING_SFX);
    }
    if (g_block[NK_CFG_MUTED]) {                      /* a muted page still has to drain the board: the
                                                       * game waits on the DMA ring, not on the device */
        static int16_t sink[1024];
        sys_audio_set_source(g_dat_63fb4);
        while (sys_audio_fill(sink, 1024) != 0) { }
        return;
    }
    sys_audio_set_source(g_dat_63fb4);
    static int16_t buf[1024];
    uint32_t got;
    /* Ask the board only for what the page can take (the configured queue depth, 60 ms by default), the way
     * the desktop host's SDL callback is bounded by the device: bytes fetched and then not queued would be
     * bytes thrown away. Bytes the board cannot drain are the reason fn_04185e stalls, so past half its ring
     * the host drains and discards rather than leaving them there (src/app/main.c's null sink, docs/129). */
    for (;;) {
        uint32_t ms     = g_block[NK_CFG_SFX_MS] ? g_block[NK_CFG_SFX_MS] : 300u;
        uint32_t used   = ring_used(NK_RING_SFX), target = ring_target(NK_RING_SFX, ms);
        if (used >= target) {
            if (sys_audio_pending() > NK_BOARD_BACKLOG_BYTES) {
                uint32_t n = 0;
                while (++n <= 32u && (got = sys_audio_fill(buf, 1024)) != 0) g_sfx_dropped += got * 2u;
            }
            return;
        }
        uint32_t room = target - used;
        got = sys_audio_fill(buf, room < 1024u ? room : 1024u);   /* the ring is at the board's rate: 1:1 */
        if (!got) return;
        uint32_t p = ring_push(NK_RING_SFX, buf, got, 1, room);
        g_sfx_bytes += p * 2u;
        if (p < got) g_sfx_deferred += (got - p) * 2u;            /* the ring filled mid-call */
    }
}

/* ---- CD-DA ------------------------------------------------------------------------
 * The MSCDEX PLAY replaces whatever was playing, so the queued bytes of the outgoing track are dropped
 * (docs/105 §1: the port used to queue the old track ahead of the new one; docs/307 measured the
 * audible result as 4-10 s of lag). The reader's budget accumulates with WALL time plus one cushion
 * granted at PLAY (docs/302: an allowance re-granted per pump call is not a lead). */
#define NK_CDDA_CUSHION_SECTORS 30u
static uint32_t g_cdda_lba, g_cdda_sectors, g_cdda_pos;
static uint64_t g_cdda_clock_us;
static uint64_t g_cdda_budget;              /* sectors of wall time, accumulated since the PLAY */
static uint64_t g_cdda_feed_us;             /* the last byte pushed to the ring; 0 = nothing pushed yet */
static uint32_t g_cdda_anchor_n;            /* re-anchors since the last byte pushed (see cdda_stuck) */

static void cdda_play(uint32_t index, uint32_t lba, uint32_t sectors) {
    ring_flush(NK_RING_CDDA);
    g_cdda_tracks++;
    g_cdda_lba = lba; g_cdda_sectors = sectors; g_cdda_pos = 0;
    g_cdda_clock_us = nk_now_us();
    g_cdda_budget = 0;                          /* the cushion is granted once, here, at the PLAY */
    g_cdda_feed_us = 0; g_cdda_anchor_n = 0;          /* the stuck report waits for a byte that has not happened yet */
    printf("cdda: track index %u lba=%u sectors=%u (%.1f s)\n", index, lba, sectors, sectors / 75.0);
    g_block[NK_ST_TRACKS] = g_cdda_tracks;
}

static void cdda_stop(void) {
    ring_flush(NK_RING_CDDA);
    g_cdda_sectors = 0; g_cdda_pos = 0;
    g_cdda_feed_us = 0; g_cdda_anchor_n = 0;
}

/* BGM MODE -- the third row of the pause menu (fn_039c03, entry m == 2) -------------------------------
 * The row's toggle handler (0x00039e30-e96, the Up/Down keys per docs/60's keymap: 0x48/0x50 ->
 * DAT_00063ed3/ed5) writes DAT_00051a10 and then either re-selects the room's track (fn_10b1d, 0x00039e8c)
 * or issues the CD request whose template is DS 0x51b50 (fn_00010dde, 0x00039e69). DAT_00051a10 has exactly
 * two readers in the whole game -- this row, and fn_10b1d, which returns at 0x010b38 when it is clear -- so
 * it is the BGM enable, and it gates the *request*, never a playhead. The port streams a whole track the
 * moment fn_10b1d asks for one, so a byte the game only consults on the next request cannot silence the one
 * already streaming: with BGM MODE off the music went on playing to its end. The original has no playhead to
 * stop either (docs/413: the MSCDEX surface is an install check and a media read), which is exactly the
 * point -- in the original the heard stream IS the game's reads, and with DAT_00051a10 clear there are no
 * reads. So the enable byte has to gate the FEED as well as the request, and this host is the feed.
 * Turning BGM back on does not restart anything (0x00039e69's fn_00010dde is a media read, which src/f
 * models as a no-op), and the stream comes back on the game's next fn_10b1d -- the next card or room. Same
 * on both sides: the original's feed restarts at its next read too. */
static int g_bgm_off_logged;
static uint64_t g_bgm_off_us;               /* 0 = off; else the moment to clear DAT_00051a10 (--bgmoff) */
static int g_bgm_off_fired;

static void cdda_bgm_gate(void) {
    if (g_dat_51a10) { g_bgm_off_logged = 0; return; }
    if (!g_bgm_off_logged) {                       /* one line per press, not one per 20 ms pump */
        printf("cdda: BGM MODE off -- dropping the %u sectors still queued\n", g_cdda_sectors - g_cdda_pos);
        g_bgm_off_logged = 1;
    }
    if (g_cdda_sectors) cdda_stop();               /* silence the ring, stop Range-reading the track */
}

/* Why the disc feed stopped when the page was the one asking for more. A host that stops feeding a ring the
 * reader has already drained is the bug a player can only describe as "the music stopped", and every early
 * return in cdda_pump is a candidate -- so a return says which, once per reason. Two guards keep it out of
 * the ordinary case: the ring has to be quarter-empty (a full ring means the READER is behind, which is
 * aud.underrun's story and not this one), and nothing may have been pushed for a second and a half (a ring
 * that is empty for one pump after a PLAY is normal and self-correcting). */
static uint64_t g_cdda_stuck_us;
static const char *g_cdda_stuck_why;
static void cdda_stuck(const char *why, uint32_t used, uint32_t target, uint32_t want, uint64_t allowed) {
    uint64_t now = nk_now_us();
    if (!g_cdda_feed_us || used * 4u >= target) return;
    if (now - g_cdda_feed_us < 1500000ull) return;
    if (g_cdda_stuck_why && !strcmp(g_cdda_stuck_why, why) && now - g_cdda_stuck_us < 5000000ull) return;
    g_cdda_stuck_why = why; g_cdda_stuck_us = now;
    printf("cdda: fed nothing for %llu ms at pos=%u/%u of lba=%u: %s -- ring=%u/%u budget=%llu allowed=%llu want=%u\n",
           (unsigned long long)((now - g_cdda_feed_us) / 1000ull), g_cdda_pos, g_cdda_sectors, g_cdda_lba, why,
           used, target, (unsigned long long)g_cdda_budget, (unsigned long long)allowed, want);
}

static void cdda_pump(void) {
    if (g_bgm_off_us && !g_bgm_off_fired && nk_now_us() >= g_bgm_off_us) {
        g_bgm_off_fired = 1;
        g_dat_51a10 = 0;
        printf("bgm: cleared DAT_00051a10 at t=%llu ms (simulates the pause menu's BGM MODE row, 0x00039e30)\n",
               (unsigned long long)((nk_now_us() - g_start_us) / 1000ull));
    }
    cdda_bgm_gate();
    if (!g_cdda_sectors) return;                    /* nothing playing, or a track that ended: said so at the time */
    if (!g_ring_words[NK_RING_CDDA][NK_RING_RATE]) {
        cdda_stuck("no device rate yet", ring_used(NK_RING_CDDA),
                   ring_target(NK_RING_CDDA, 120u), 0u, 0ull);
        return;                                     /* no AudioContext yet: nothing to feed,
                                                     * and losing the read position would drop
                                                     * the first seconds of the track */
    }
    uint64_t now = nk_now_us();
    uint64_t el_sec = (now - g_cdda_clock_us) * 75ull / 1000000ull;      /* sectors of wall time */
    g_cdda_clock_us += el_sec * 1000000ull / 75ull;
    g_cdda_budget += el_sec;                      /* src/app/main.c:385 -- the budget accumulates, the
                                                   * clock is what gets consumed */
    uint32_t allowed = (uint32_t)g_cdda_budget + NK_CDDA_CUSHION_SECTORS;
    if (allowed <= g_cdda_pos) {
        cdda_stuck("budget behind the read position", ring_used(NK_RING_CDDA),
                   ring_target(NK_RING_CDDA, 120u), 0u, allowed);
        return;
    }
    /* Re-anchor rather than race when the wall clock ran away from the reader -- a backgrounded tab, which
     * stops requestAnimationFrame and therefore stops the game's tick clock too, or a muted page, which stops
     * its own reader (docs/462). The game's own CD bookkeeping runs off ticks, so it still believes the track
     * is where the reader left it: letting the reader sprint to catch the wall clock would be the one case
     * where the heard music is wrong, and it would stream at 10x real time doing it.
     *
     * Drop the surplus and start the clock HERE. The first version of this anchored the clock a cushion-plus-
     * position behind `now` as well as zeroing the budget, which credited those sectors a second time on the
     * very next pump -- `allowed` then exceeded `pos + 2*CUSHION` by `pos + elapsed`, which is true forever, so
     * the branch armed itself and the disc feed never ran again. Measured: a 6 s mute armed it, and CD-DA was
     * dead for the rest of the session at pos=9, with the ring the reader had already drained. */
    if (allowed > g_cdda_pos + NK_CDDA_CUSHION_SECTORS * 2u) {
        g_cdda_clock_us = now;
        g_cdda_budget = g_cdda_pos;                   /* nothing owed beyond what has actually been read */
        /* Counted rather than reported on sight: ONE re-anchor after a backgrounded tab or a mute is the
         * safety net doing its job, and the next pump feeds. Reporting that would make a legitimate recovery
         * look like the bug, and `NIGHT_CHECK` fails on this line. Two in a row means the anchor did not
         * help -- which is precisely what the old arithmetic did forever. */
        if (++g_cdda_anchor_n >= 2u)
            cdda_stuck("re-anchored, and again on the next pump", ring_used(NK_RING_CDDA),
                       ring_target(NK_RING_CDDA, 120u), 0u, allowed);
        return;
    }

    uint32_t room_frames = ring_room(NK_RING_CDDA);
    uint32_t target      = ring_target(NK_RING_CDDA, g_block[NK_CFG_CDDA_MS] ? g_block[NK_CFG_CDDA_MS] : 120u);
    if (ring_used(NK_RING_CDDA) >= target) return;
    if (room_frames > target - ring_used(NK_RING_CDDA)) room_frames = target - ring_used(NK_RING_CDDA);
    room_frames = (room_frames / 588u) * 588u;      /* whole sectors only */
    if (!room_frames) return;

    uint32_t want_sec = room_frames / 588u;
    uint32_t want = sys_cd_da_plan(g_cdda_pos, g_cdda_sectors, want_sec, 16u);
    if (!want) { cdda_stuck("the game's plan asked for none", ring_used(NK_RING_CDDA), target, 0u, allowed); return; }
    for (uint32_t i = 0; i < want; i++) {
        if (cd_iso_read_audio(g_cdda_lba + g_cdda_pos, g_cdda_sector) != 0) {
            /* The one way CD-DA ends before its sectors do, and after docs/460 it is only ever the real
             * end: the byte source answers a missing window with silence, so a read that comes back
             * refusing means the track has no more bytes, or no file, or a server that does not do Range.
             * Loud, because to a player this is the music stopping mid-room with nothing to bring it back:
             * nothing here re-PLAYs a track, only the game's next fn_10b1d does. */
            printf("cdda: read failed at pos=%u/%u of lba=%u -- the track is abandoned\n",
                   g_cdda_pos, g_cdda_sectors, g_cdda_lba);
            g_cdda_sectors = 0; break;
        }
        uint32_t p = ring_push(NK_RING_CDDA, (const int16_t *)g_cdda_sector, 588u, 2,
                               ring_room(NK_RING_CDDA));
        g_cdda_bytes += p * 4;
        g_cdda_pos++;
        if (p) { g_cdda_feed_us = now; g_cdda_anchor_n = 0; }
        if (p < 588u) break;                        /* the ring is full again: stop reading ahead */
    }
}

/* ---- the tick pump ---------------------------------------------------------------- */
/* Same numbers src/app/main.c prints with --dump, so a wasm frame and a desktop frame can be compared
 * without a screenshot: how many indices the screen uses, how much of the DAC is lit, whether the writes
 * are 6-bit or 8-bit, and a hash of a strided arena sample that changes when the picture changes. */
static void video_stats(uint32_t n) {
    int vw = g_view_320x200 ? 320 : sys_video_w(), vh = g_view_320x200 ? 200 : sys_video_h();
    static uint8_t seen[256];
    memset(seen, 0, sizeof seen);
    for (int i = 0; i < vw * vh; i++) seen[g_video_ram[i]] = 1;
    int ndx = 0, lit = 0, dac_max = 0, dac_gt63 = 0;
    for (int i = 0; i < 256; i++) if (seen[i]) ndx++;
    for (int i = 0; i < 768; i++) { if (g_palette8[i] > dac_max) dac_max = g_palette8[i]; if (g_palette8[i] > 63) dac_gt63++; }
    for (int i = 0; i < 768; i += 3) if (g_palette8[i] | g_palette8[i + 1] | g_palette8[i + 2]) lit++;
    uint64_t hash = 1469598103934665603ull;
    for (int i = 0; i < 307200; i += 193) hash = (hash ^ g_video_ram[i]) * 1099511628211ull;
    printf("video %u: %dx%d colours=%d dac lit=%d writes=%u dacmax=%d dacle64=%d "
           "pal0=%u/%u/%u pal1=%u/%u/%u pal255=%u/%u/%u arena=%016llx\n",
           n, vw, vh, ndx, lit, sys_vga_palette_writes(), dac_max, dac_gt63,
           g_palette8[0], g_palette8[1], g_palette8[2],
           g_palette8[3], g_palette8[4], g_palette8[5],
           g_palette8[765], g_palette8[766], g_palette8[767],
           (unsigned long long)hash);
}

static void tick_pump(void) {
    uint64_t now = nk_now_us();

    /* How long the game held the thread between two pumps. In the browser this is the thing a desktop host
     * never shows you: the game's step, the disc reads and the yield all happen on the one UI thread, so a
     * long gap is a long frozen picture -- and the number separates "the page stopped being driven" from
     * "this step genuinely took". */
    if (g_last_pump_us) {
        uint64_t gap = now - g_last_pump_us;
        g_gap_us += gap;
        if (gap > g_gap_max_us) g_gap_max_us = gap;
        if (gap > 500000ull)
            printf("stall: %u ms between pumps at frame %u tick %u\n",
                   (uint32_t)(gap / 1000ull), g_frames, g_dat_63e24);
    }
    g_pumps++;
    g_last_pump_us = now;

    if (now >= g_audio_next_us) {
        g_audio_next_us = now + 20000ull;           /* a 20 ms cadence, as on the desktop host */
        /* What the page's audio looks like from inside, for the HUD and the smoke test to read. */
        g_block[NK_ST_FLAGS] = (g_block[NK_CFG_MUTED] ? 0u : 1u)
                             | (g_block[NK_CFG_AUDIO_RATE] ? 2u : 0u);
        audio_pump();
        cdda_pump();
        g_block[NK_ST_SFX_BYTES] = g_sfx_bytes;
        g_block[NK_ST_CDDA_BYTES] = g_cdda_bytes;
    }

    if (g_block[NK_CFG_WANT_QUIT] && !g_quit) {
        printf("quit: the page asked to leave\n");
        g_quit = 1; g_exit_state = 1;
        if (!g_exit_deadline_us) g_exit_deadline_us = now + 3000000ull;
    }
    nk_exit_check();

    if (now >= g_diag_next_us) {
        uint32_t rd = 0, rb = 0, ft = 0, fb = 0, ff = 0, wt = 0, wm = 0, sil = 0;
        nk_disc_stats(&rd, &rb, &ft, &fb, &ff, &wt, &wm, &sil);
        printf("pump: t=%llums frames=%u ticks=%u tick=%u stage=%u audio=%uHz pend=%u xfer=%u stops=%u "
               "sfx=%uB/%uB dropped/%uB deferred (%u rate changes marked, %u flushed: %uB) cdda=%uB tracks=%u pos=%u "
               "bgm=%u snd=%u vgaws=%uns/%uB charged "
               "ring=%u/%u@%u dev=%u at stop n=%u board avg=%u max=%u page avg=%u max=%u "
               "disc reads=%u fetched=%u/%uB fails=%u waited=%u(max %ums) silence=%uB\n",
               g_start_us ? (unsigned long long)((now - g_start_us) / 1000ull) : 0ull,
               g_frames, g_timer_calls, g_dat_63e24, g_dat_63370,
               sys_audio_rate(), sys_audio_pending(), sys_audio_transfers(), sys_audio_stops(),
               g_sfx_bytes, g_sfx_dropped, g_sfx_deferred, g_sfx_marks, g_sfx_flushes, g_sfx_chop_bytes,
               g_cdda_bytes, g_cdda_tracks, g_cdda_pos,
               (unsigned)g_dat_51a10, (unsigned)g_dat_53338, g_vga_window_ns,
               (unsigned)(g_vga_window_bytes > 0xffffffffull ? 0xffffffffu : (uint32_t)g_vga_window_bytes),
               ring_used(NK_RING_SFX), ring_used(NK_RING_CDDA), g_ring_words[NK_RING_CDDA][NK_RING_RATE],
               g_block[NK_CFG_AUDIO_RATE],
               g_stop_n, g_stop_n ? g_stop_board / g_stop_n : 0, g_stop_max_board,
               g_stop_n ? g_stop_page / g_stop_n : 0, g_stop_max_page, rd, ft, fb, ff, wt, wm / 1000u, sil);
        video_stats(g_frames);
        /* Per-window numbers, each divided by what this window actually did -- frames counted here and not
         * since boot, or the second window's average is a lie. */
        printf("cost: pumps=%u avg_gap=%lluus max_gap=%lluus frames=%u | step=%lluus/tick "
               "present=%lluus/frame yield=%lluus/frame (max %lluus)\n",
               g_pumps, (unsigned long long)(g_gap_us / (g_pumps ? g_pumps : 1)),
               (unsigned long long)(g_gap_max_us), g_frames_win,
               (unsigned long long)(g_tick_calls ? g_step_us / g_tick_calls : 0),
               (unsigned long long)(g_frames_win ? g_present_us / g_frames_win : 0),
               (unsigned long long)(g_frames_win ? g_yield_us / g_frames_win : 0),
               (unsigned long long)(g_yield_max_us));
        g_pumps = 0; g_gap_us = 0; g_gap_max_us = 0; g_step_us = 0; g_tick_calls = 0;
        g_frames_win = 0;
        g_present_us = 0; g_yield_us = 0; g_yield_max_us = 0;
        g_diag_next_us = now + g_diag_us;
    }

    if (now >= g_tick_next_us) {
        uint32_t hz = sys_pit_hz(0);                /* fn_03ef29 programs PIT ch0 (0x982 -> 490 Hz) */
        if (hz == 0 && g_dat_63318) hz = 1193182u / g_dat_63318;   /* fn_03575d's ch2 user */
        if (hz == 0) hz = 18;
        uint64_t period = 1000000ull / hz;
        if (now - g_tick_next_us > period * 8) g_tick_next_us = now;          /* fell behind: re-anchor */
        uint32_t calls = 0;
        while (now >= g_tick_next_us && calls < 64) { g_tick_next_us += period; calls++; }
        uint32_t n = calls;
        uint64_t t0 = nk_now_us();
        while (calls--) { fn_03ee88(); g_timer_calls++; }
        g_step_us += nk_now_us() - t0;
        g_tick_calls += n;
    }

    if (now >= g_present_next_us) {
        uint64_t t0 = nk_now_us();
        present();
        g_present_us += nk_now_us() - t0;
        g_frames++;
        g_frames_win++;
        g_block[NK_ST_FRAMES] = g_frames;
        g_present_next_us = now + NK_FRAME_US;
        t0 = nk_now_us();
        nk_yield("frame");                          /* the browser gets its paint here */
        uint64_t w = nk_now_us() - t0;
        g_yield_us += w;
        if (w > g_yield_max_us) g_yield_max_us = w;
    } else {
        nk_yield("watchdog");
    }
}

/* ---- boot ------------------------------------------------------------------------ */
/* The sound board is only initialised if the hardware answers (docs/98: the SB16 detection probe, the
 * 0xf2 IRQ and the 0xe1 id read); without it DAT_00063f90 stays 0 and every in-game SFX call is
 * swallowed. Copied from src/app/main.c's board_boot() and pocket/src/night/main_of.c's copy of it. */
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
    sys_isr_bind(0x41486, fn_041486);
    sys_isr_bind(0x414d3, fn_0414d3);
    sys_isr_bind(0x41520, fn_041520);
    sys_isr_bind(0x4156d, fn_04156d);
    sys_dev_cmd_hook(board_cmd_hook);
    g_dat_53334 = 0x220;                              /* the value the original runs with at the title */
}

static void block_init(void) {
    /* Zero what the host owns and what the ring headers need, but leave words NK_CFG_AUDIO_RATE..
     * NK_CFG_WORDS alone: the page writes the AudioContext rate into that window as soon as it has a
     * context, which is BEFORE callMain gets here. Clearing it would put the device rate to sleep until
     * the next write, and the rings would sit at the default rate for no reason. */
    memset(g_block, 0, NK_CFG_AUDIO_RATE * 4u);
    memset(&g_block[NK_CFG_WORDS], 0, (NK_KEY_WORDS - NK_CFG_WORDS) * 4u);
    g_block[NK_KEY_SIZE] = NK_KEYQ_SLOTS;
    g_block[NK_KEY_SLOT_BYTES] = NK_KEY_WORDS * 4u;
    for (int w = 0; w < 2; w++) {
        uint32_t *I = g_ring_words[w];
        I[NK_RING_CAP]  = NK_RING_FRAMES;
        I[NK_RING_MASK] = NK_RING_FRAMES - 1u;
        I[NK_RING_PCM]  = (uint32_t)(uintptr_t)g_ring_pcm[w];
        I[NK_RING_RATE] = NK_DEFAULT_RATE;
        I[NK_RING_MARK_GEN] = 1u;                       /* 0 would look like "never flushed" to a reader */
        ring_marks_seed(w, 0u, NK_DEFAULT_RATE);
    }
}

/* Print the layout the page mirrors, so a drift between wasm_block.h and night.js shows up in the
 * console as numbers that do not match rather than as a wrong byte somewhere. */
static void block_layout_log(void) {
    printf("block: base=%u status[0..15] cfg@%u keys{head=%u tail=%u size=%u slots@%u} rings{sfx=%u cdda=%u}\n",
           nk_block_addr(), NK_CFG_AUDIO_RATE, NK_KEY_HEAD, NK_KEY_TAIL, NK_KEY_SIZE, NK_KEY_SLOT_BYTES,
           nk_ring_addr(0), nk_ring_addr(1));
}

int main(int argc, char **argv) {
    const char *res_dir = "/data";
    const char *cue     = "/data/TG_NIGHT.CUE";
    const char *cd_dir  = "/data/cd";      /* the directory cd_iso names as its byte source */
    const char *cd_url  = "data/cd";       /* the same directory as the page sees it, for Range reads */
    int sound_on        = 1;
    /* The A0000 window-write charge, in ns/byte -- the thing that paces the melting title, the logo fade and
     * the room's page flip (docs/72, docs/76). This host had never set it, so g_vga_window_ns stayed at its
     * unit-test default of 0 and every one of those wrote free: fn_03a224's wipe is 240 passes x 307200 window
     * bytes and ran through in under a second (charged in 0.65 s, measured), where the original's own captured
     * frames put it at 7.19 s -- the "intro almost skipped" report. Default 98 is the physical figure docs/72
     * pins (97.5 ns/byte = two 486 cycles per uncacheable VGA write); the browser can afford the physical
     * number because its waits are coalesced below, so the charge itself is what spends the time. Measured with
     * `?diagms=200`, from the first charged byte to the 73,728,000th: 6.4-6.8 s at 80 ns/byte (ratio 0.89-0.95)
     * and 7.5-7.8 s at 98 (1.04-1.08), against the original's 7.19 s. The desktop host stays at its own tuned
     * 63, which is 63 only because its per-scanline waits overshoot (docs/72). `?vgaws=0` gets the free writes
     * back for an A/B. */
    uint32_t vgaws        = 98u;
    /* How much window-write debt is worth a wait (g_vga_charge_spin_us, f.c). Measured here: with a wait on
     * every scanline (0, the desktop host's setting) the wipe ran at 48 ms/pass -- 2.5x the 19.35 ms the model
     * asks for at 63 ns/byte -- because each of the 480 waits per pass runs a pass of the pump as well, and the
     * pump is the audio pump, a present and (at the frame boundary) the wait for requestAnimationFrame.
     * Coalescing the waits at 2 ms of debt brought the span onto the charge itself, which is what makes the
     * physical charge the right default above (docs/459). `?chargeus=0` asks for the per-scanline behaviour. */
    uint32_t chargeus     = 2000u;
    /* A test knob, not a feature: clear the BGM enable byte (DAT_00051a10) N ms into the run, which is exactly
     * what the pause menu's BGM MODE row does (fn_039c03, 0x00039e30-e96) -- see cdda_bgm_gate(). It exists
     * because a headless run cannot reach that row: a browser run has to get past the episode's stage card and
     * into the room loop first (docs/459's Not claimed), and it has not managed to. So the gate is exercised
     * the same way the menu would, one press simulated at a known time. `?bgmoff=15000`. */
    uint32_t bgmoff_ms    = 0u;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--res-dir") && i + 1 < argc)  res_dir = argv[++i];
        else if (!strcmp(argv[i], "--cue") && i + 1 < argc) cue = argv[++i];
        else if (!strcmp(argv[i], "--cd-dir") && i + 1 < argc) cd_dir = argv[++i];
        else if (!strcmp(argv[i], "--cd-url") && i + 1 < argc) cd_url = argv[++i];
        else if (!strcmp(argv[i], "--vgaws") && i + 1 < argc) vgaws = (uint32_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--chargeus") && i + 1 < argc) chargeus = (uint32_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--bgmoff") && i + 1 < argc) bgmoff_ms = (uint32_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--diagms") && i + 1 < argc) g_diag_us = (uint64_t)atoi(argv[++i]) * 1000ull;
        else if (!strcmp(argv[i], "--no-sound")) sound_on = 0;
        /* The desktop host's debug flags, same names: get to the title card (and so to a room) without
         * sitting through the logo hold and the opening movie. A browser run that has to reach the room loop
         * before it can open the pause menu wants this. */
        else if (!strcmp(argv[i], "--skip-intro")) g_skip_intro = 1;
    }
    g_vga_window_ns = vgaws;
    g_vga_charge_spin_us = chargeus;

    block_init();
    printf("NIGHT (wasm) -- SDL-free browser host. docs/457\n");
    printf("window charge: %u ns/byte (0 = every A0000 write is free, and the intro paces themselves)\n",
           g_vga_window_ns);
    block_layout_log();

    /* The page's AudioContext rate arrives as data, not as a call: it may only exist by the time the
     * first pump runs (autoplay policy), and the rings must not be fed before it does. */
    for (int w = 0; w < 2; w++) g_ring_words[w][NK_RING_RATE] = NK_DEFAULT_RATE;   /* the page may correct it */

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
    for (int t = 0; t < 0x104; t++) { g_dat_639ec[t] = 0; g_dat_53818[t] = 0; }
    if (sound_on) g_dat_53338 = 1;                    /* the title menu's SOUND toggle (fn_039c03) */
    sys_cd_card_stop_enable(1);                       /* docs/413: quiet over the stage-card hold, as the
                                                       * original is. Opt-in per host; sys.c defaults it off. */

    /* Resource opens (fn_04270d / fn_036853) resolve to "<dir>/<name>"; the page has put NIGHT.RES at
     * <res_dir>/NIGHT.RES in the in-memory filesystem before calling main. */
    sys_file_real_dir(res_dir);
    printf("res dir: %s\n", res_dir);

    /* The disc: the CUE's TOC parses from a 669-byte file, its IMG does not exist (there is no image in
     * the browser), so cd_iso falls back to the per-track directory -- whose bytes are answered by HTTP
     * Range reads. NIGHT_CD_DIR is the documented knob that names that directory. */
    setenv("NIGHT_CD_DIR", cd_dir, 1);
    nk_disc_install(cd_url);
    if (cd_iso_open(cue) == 0) {
        sys_cd_set_backend(cd_iso_root_header, cd_iso_root_records);
        printf("disc: %u tracks, %u sectors, audio from %s (%d of %d audio tracks have no bytes)\n",
               cd_iso_track_count(), cd_iso_total_sectors(), cd_iso_source(),
               cd_iso_tracks_missing(), cd_iso_tracks_total());
        g_block[NK_ST_DISC] = 1;
        if (cd_iso_has_audio()) sys_cd_set_audio_backend(cdda_play, cdda_stop);
        else printf("disc: no track bytes are answerable -- the game runs silent\n");
    } else {
        printf("disc: %s is not readable, and nothing is substituted for it: fn_010c3e will show "
               "cdinsert.pcx and wait, as the original does\n", cue);
    }

    board_boot();
    sys_set_input_pump(input_pump);
    sys_set_tick_pump(tick_pump);
    sys_set_present_hook(present);        /* docs/325: a pass that writes 0xA0000 and waits asks for one present */

    g_start_us = nk_now_us();
    if (bgmoff_ms) g_bgm_off_us = g_start_us + (uint64_t)bgmoff_ms * 1000ull;
    g_tick_next_us = g_start_us;
    g_present_next_us = g_start_us;
    g_audio_next_us = g_start_us + 20000ull;
    g_diag_next_us = g_start_us + g_diag_us;
    g_yield_next_us = g_start_us;
    g_block[NK_ST_BOOTED] = 1;

    printf("game main: entered\n");
    dispatch_a(0xff);                                 /* 0x422c7: the registered event-table entries */
    fn_0445c9_startup();                              /* 0x0445d8-0x0445fe: the entry thunk's pre-main half */
    fn_03af18();                                      /* THE GAME MAIN */
    printf("the game main returned\n");
    g_quit = 1;
    g_exit_state = 2;
    g_block[NK_ST_EXIT] = 2;
    present();
    printf("frames presented: %u  ticks: %u  cdda tracks: %u\n", g_frames, g_timer_calls, g_cdda_tracks);
    return 0;
}

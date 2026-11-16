/* wasm_block.h -- the one memory block the browser host and runtime/night.js share.  docs/457.
 *
 * WHY A BLOCK AND NOT CALLS
 * -------------------------
 * The browser build runs the game main under -sASYNCIFY: the game's whole call stack is unwound into
 * the asyncify stack every time the host yields, so the browser can paint, take events and run the
 * audio callback. In that window the wasm instance is mid-unwind and MUST NOT be entered again -- a
 * second entry (a keydown handler, an AudioContext callback) would write its own frames into the same
 * unwind bookkeeping and corrupt the sleeping stack. So nothing in night.js calls into wasm except the
 * one call that resumes a yield. Everything else crosses through this block, which is plain memory:
 * JS writes bytes into it (keys, mute, quit) and reads bytes out of it (status, the PCM rings), and the
 * only JS->wasm entry is nk_yield_raf() resolving.
 *
 * The layout below is the contract; night.js mirrors the same indices and the host prints them at boot
 * so a drift shows up in the console instead of as a wrong byte somewhere.
 */
#ifndef NK_WASM_BLOCK_H
#define NK_WASM_BLOCK_H

#include <stdint.h>

/* ---- the shared block ---- */
enum {
    /* C -> JS, words 0..15: what the host is doing */
    NK_ST_BOOTED      = 0,   /* 1 once the game main has been entered */
    NK_ST_FRAMES      = 1,   /* frames presented */
    NK_ST_VID_W       = 2,   /* the geometry last composed */
    NK_ST_VID_H       = 3,
    NK_ST_EXIT        = 4,   /* 0 running, 1 asked to leave, 2 the game main returned */
    NK_ST_TRACKS      = 5,   /* CD-DA PLAY requests seen */
    NK_ST_SFX_BYTES   = 6,   /* board PCM bytes handed to the ring */
    NK_ST_CDDA_BYTES  = 7,   /* CD-DA bytes handed to the ring */
    NK_ST_DISC        = 8,   /* 1 = a byte source answers the reads */
    NK_ST_FLAGS       = 9,   /* bit0 sound on, bit1 audio device live */
    NK_ST_KEYS        = 10,  /* key events applied to the game's key bytes */
    /* Two honest numbers, because "KB streamed" had been lying by construction the moment bytes could be
     * made without a network: silence for a dropped read, silence for a soundtrack the visitor turned off,
     * and -- in pre-download mode -- the whole score from memory. NK_ST_CDDA_BYTES counts bytes handed to
     * the ring, which is what the game consumed; this counts bytes that came off the wire, which is what
     * the player's data plan consumed. They are equal only in streamed mode. */
    NK_ST_NET_BYTES   = 11,  /* bytes fetched over the network (Range windows that landed) */
    NK_ST_MUSIC       = 12,  /* 1 = the soundtrack is being played, 0 = asked for silence instead */
    NK_ST_WORDS       = 16,

    /* JS -> C, words 16..31: what the page asked for */
    NK_CFG_AUDIO_RATE = 16,  /* the AudioContext's sample rate, 0 until one exists */
    NK_CFG_MUTED      = 17,  /* 1 = stop feeding the rings (the game still drains its board) */
    NK_CFG_WANT_QUIT  = 18,  /* JS asked the game to leave */
    NK_CFG_FLAGS      = 19,  /* bit0 = sound effect output wanted */
    /* How deep a queue the host keeps in each ring, in milliseconds, 0 = the host's own default (60 for the
     * board, 120 for the disc). It is the page's to set because it is the page that has to survive its own
     * stalls: the callback is serviced by the main thread, which the game holds for tens of milliseconds at
     * a time, and a queue shallower than the worst stall is heard as a hole. `?sfxms=` sets it. */
    NK_CFG_SFX_MS     = 20,
    NK_CFG_CDDA_MS    = 21,
    NK_CFG_WORDS      = 32,

    /* key queue header, words 32..39, slots from word 40 */
    NK_KEY_HEAD       = 32,  /* JS writes: next slot to fill */
    NK_KEY_TAIL       = 33,  /* C  writes: next slot to read */
    NK_KEY_OVER       = 34,  /* dropped because the queue was full */
    NK_KEY_SIZE       = 35,  /* slot count (power of two) */
    NK_KEY_SLOT_BYTES = 36,  /* byte offset of slot 0 from the block base */
    NK_KEY_WORDS      = 40,

    /* each slot is one uint32: scancode in the low byte, make/break in the next */
    NK_KEYQ_SLOTS     = 512
};

/* ---- one PCM ring (there are two: 0 = the sound board, 1 = CD-DA) ----
 * C produces, JS consumes, and they are the same thread -- the audio callback and the game step never
 * interleave, so free-running counters and no locks. */
enum {
    NK_RING_HEAD = 0,        /* C  writes: frames produced (free-running) */
    NK_RING_TAIL = 1,        /* JS writes: frames consumed (free-running) */
    NK_RING_CAP  = 2,        /* frames, power of two */
    NK_RING_MASK = 3,
    NK_RING_RATE = 4,        /* the sample rate the OLDEST frames still in it are at */
    NK_RING_PCM  = 5,        /* byte address of int16[cap*2], L/R interleaved */
    NK_RING_MARK_GEN = 6,    /* C  writes: +1 whenever the ring is flushed, so JS can drop its reader */
    NK_RING_WORDS = 8
};
enum { NK_RING_SFX = 0, NK_RING_CDDA = 1 };

/* ---- rate marks: where a ring changes sample rate ----
 * The board's rate is programmed per DMA transfer, and the game programs 10000, 10989, 11111 and 22050 Hz
 * in a single scene. The ring runs at whatever rate the bytes were written at, so a rate change in the
 * middle of a queued run has to be *recorded*, not acted on: the desktop host hands the new rate to
 * SDL_SetAudioStreamFormat and SDL keeps the queued bytes under the old spec. Until now this host flushed
 * the ring instead, which discarded the tail of one sound and the head of the next -- 1621 frames (37 ms)
 * in a 34 s run, on 4 of its 6 rate changes. A mark says "frame F onwards is at rate R", and the mixer
 * switches ratio at exactly F.
 *
 * The table is a ring: entry ordinal o sits at slot o % NK_RING_MARK_CAP, and
 * [firstOrdinal, firstOrdinal + count) is live.
 *
 *   [0]  count              [1]  firstOrdinal      [2 + 2i] frame, [3 + 2i] rate of slot i
 *
 * Entry firstOrdinal is the BASE: the rate the ring has held since its earliest live frame, seeded at boot
 * and re-seeded on a flush. That is not a nicety. The page has to know the rate of the frames it starts on,
 * and if the newest change is also the only entry there is nothing to hold it back with -- so a reader that
 * treats the last entry as not-yet-applyable plays every sound at the *previous* transfer's rate. It did,
 * and the first sound of a session ran at ratio 1.0 instead of 0.25: the ring drained four times faster
 * than the game filled it, which is heard as "I can't hear most sounds".
 *
 * The reader consumes marks and always sits ahead of the ring's tail, so C drops entries the page has
 * passed from the front and they cost nothing. They used to be kept forever, which meant the 64 slots
 * filled in about a minute and every change after that flushed audio.
 */
enum {
    NK_RING_MARK_CAP   = 64,          /* entries that fit; a power of two, the slot is ordinal % cap */
    NK_RING_MARK_N     = 0,           /* entries present */
    NK_RING_MARK_ORD   = 1,           /* absolute ordinal of the entry in slot 0 */
    NK_RING_MARK_ENTRY = 2,           /* entries begin here: (frame, rate) pairs */
    NK_RING_MARK_EWORDS = 2,          /* words per entry */
    NK_RING_MARK_STRIDE = NK_RING_MARK_ENTRY + NK_RING_MARK_CAP * NK_RING_MARK_EWORDS
};

/* A key event as it sits in one queue slot. */
#define NK_KEY_SLOT(sc, make) ((uint32_t)(sc) | ((uint32_t)(((make) ? 1 : 0)) << 8))

#endif

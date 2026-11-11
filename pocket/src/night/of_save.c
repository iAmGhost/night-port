/* of_save.c -- save slots and settings for the Pocket host (task-7, docs/334).
 *
 * WHAT A SAVE IS HERE
 *   NIGHT has no save feature of its own: nothing in the shipped code writes a progress file (docs/06
 *   §4, docs/65), so this is a port-side feature -- a *stage checkpoint*.  It stores the two things the
 *   port can restore faithfully: which stage the run was in (the game's own stage counter
 *   DAT_00063370, which the port's host-warp already understands, docs/52/docs/322) and the player
 *   record the stage loaded (the entity block at DAT_00053c7c plus the DAT_63374..63384 mirrors of its
 *   state bytes).  On restore the host warps into that stage the way a warp run does, waits for the
 *   stage loop to be live, and writes the record back.  A full memory dump is deliberately NOT attempted:
 *   the stage data lives in the 16 MiB heap arena, which no 256 KB slot can hold, and half of it is
 *   reloaded from the RES slot at boot anyway.
 *
 * HOW A SLOT IS OPENED
 *   Bound data slots appear as flat entries in the kernel's virtual root, and the only reliable way to
 *   find one is readdir("/") plus d_ino: `d_ino - 1 == slot id` (the rule openfpgaOS-Quake's sys_of.c
 *   records, and the same one of_disc.c follows for the CD track slots).  Opening a guessed
 *   "<dir>/night_3.sav" path instead can wedge the slot bridge, so it is not tried.
 *   The desktop build has no slot bridge, so its slots are ordinary files under NIGHT_SAVE_DIR.
 */
#include "of_save.h"

#include "f/f.h"
#include "sys/sys.h"

#include <stdio.h>
#include <sys/stat.h>   /* mkdir for the desktop save directory (docs/390) */
#include <stdlib.h>
#include <string.h>

#ifndef OF_PC
#include "of.h"
#include "of_config.h"
#include "of_interact_idx.h"        /* generated: which position in interact.json each knob sits at */
#include "of_log.h"                  /* bridge probes are logged: the Pocket has no console */

/* The host's NLOG writes the card log and the console; of_save.c needs the same, and the console half is what
 * makes a desktop run readable. */
#define SLOG(...) do { of_log_line(__VA_ARGS__); printf(__VA_ARGS__); } while (0)
#include <dirent.h>
#endif

static int      g_music_vol = 255, g_sfx_vol = 255, g_cdda_on = 1, g_pad_on = 1;
static int      g_master_vol = 255;              /* the mixer output stage, never set by this port until 0.1.51 */
static int      g_sfx_gain = 200, g_cdda_gain = 150;   /* percent, through of_gain16()'s knee (docs/361) */
static int      g_tick_hz   = 0;   /* 0 = whatever PIT rate the game programs; see of_setting_tick_hz */
static unsigned g_lcd_raw   = 0xDEADBEEFu;   /* 0xDEADBEEU until the bridge is read */
static int      g_music_bridge = -1;         /* the raw interact word behind Music Volume */
/* The values interact.json actually offers (mk_dist.py writes the same lists).  A menu variable is a word in
 * the interact bridge, and at boot the bridge can carry 0 for a variable nobody has touched -- 0 is a legal
 * volume (mute) and a legal "no cap", so the host only accepts a value it can name.  That is why the two
 * knobs are lists with named options and no 0 in them: an unwired 0 must not silently mute the music. */
/* Dead since 0.1.34, kept as the record of what the decoders used to accept: the interact ecall handed back a
 * whole word (and the OS filled it with a fixed table of junk), so these two helpers tried to tell "the menu said
 * nothing" from "the menu said something odd".  Fields inside the control word are bounded by construction, so
 * nothing here needs to guess. */
#if 0
static int in_opts(const int *t, int n, int v) { for (int i = 0; i < n; i++) if (t[i] == v) return 1; return 0; }
#endif

/* 0.1.30 declares no app knobs in interact.json (docs/342: the interact read answers garbage on this OS -- every
 * index, error half included).  Their index constants therefore may be absent from the generated header, and the
 * wiring below keeps them compiled but inert so re-enabling a knob is one flag in mk_dist.py, not an edit here. */
#ifdef OF_IDX_MUSIC
#define HAVE_MUSIC 1
#else
#define HAVE_MUSIC 0
#endif
#ifdef OF_IDX_FIT
#define HAVE_FIT 1
#else
#define HAVE_FIT 0
#endif



/* Fields inside 0xF7000000.  interact.json declares them with a mask whose 1-bits are "leave untouched", so the
 * element's own bits are the complement: Music Volume 24-26 (mask 0xF8FFFFFF), Screen Fit 27-28 (mask
 * 0xE7FFFFFF), Pocket LCD 16-17 (mask 0xFFFCFFFF, the SDK core's own entry).  mk_dist.py and check_interact.py
 * own the bit map; these constants must agree with it or the menu and the host disagree about the same word. */
/* The retired control-word shape (the 0.1.34 attempt: Music Volume in bits 24-26, Screen Fit in bits 27-28 of
 * 0xF7000000).  Those decoders stay in the file because the bridge read they fed is what 0.1.37 proved fatal, and
 * their field positions are the record of what interact.json declared then.  They are named BRIDGE_* so they cannot
 * be confused with the live periph-slot decode in main_of.c, which is what check_interact.py cross-checks. */
#define BRIDGE_MUSIC_SHIFT 24
#define BRIDGE_MUSIC_MASK  0x7u
#define BRIDGE_FIT_SHIFT   27
#define BRIDGE_FIT_MASK    0x3u
#define PLCD_SHIFT  16
#define PLCD_MASK   0x3u

/* An openfpgaOS app may not read the APF status page.  Measured on hardware, not inferred: 0.1.37 bracketed every
 * probe, ran with DIAG=15, and the card log ends at
 *      probe: control-word read begin
 * with no "done" and no game -- the read of 0xF7000000 never returned, and the player watched a frozen core.  The
 * page is written BY the core for the host to read; reading it from the SoC side is not a supported operation on
 * this bitstream, and it is not a value we can buy at any cost.  So:
 *   - nothing in this port reads 0xF7000000, ever, and there is no flag that turns it back on;
 *   - Pocket LCD stays what it always was: a switch the CORE answers, which works perfectly well without the app
 *     knowing it moved (the player just confirmed exactly that on 0.1.36);
 *   - the periph page at 0x400000xx remains the one channel proven readable -- 0.1.35's scan completed and
 *     0x40000098 matched caps->hw_features exactly -- and it is where the Pocket LCD measurement still has to be
 *     made.  That is what DIAG's periph bit is for. */


/* Turn what the bridge carries into a volume, or 0 for "the menu is not carrying this knob".  Two shapes are
 * accepted on purpose: the value the list writes (1..5, one per option, which is what interact.json declares),
 * and a bare volume (64, 128, 192, 255) in case a future host decides the menu should carry levels directly.
 * 0 is never a volume here: 0 means "nothing was written" -- the options are anchored at 1 so a real choice can
 * not spell 0, and the card's own interact_persist.json showed an untouched variable reading 0. */
#define BRIDGE_JUNK 0x55555555u        /* what 0.1.28's card log found at EVERY interact index */

/* of_interact_get() is `return of_ecall1(...).value;` -- the error half of the SBI return is thrown away.  The
 * card's log said 0x55555555 for all eight indices: not a menu value, not a volume, and not one of the codes in
 * of_error.h (those are small negatives).  Error and value together say whether the OS answered junk or refused
 * the call; the value alone cannot.  Every interact read in this host goes through here now. */
long of_interact_probe(int index, unsigned *value) {
#ifdef OF_PC
    (void)index; if (value) *value = 0u; return 0;
#else
    struct of_sbiret r = of_ecall1(OF_EID_INTERACT, OF_INTERACT_FID_GET, index);
    if (value) *value = (unsigned)r.value;
    return (long)r.error;
#endif
}

#if 0   /* same reason: no field decoding needs a junk filter any more (docs/344) */
static int bridge_usable(unsigned raw) { return raw != 0u && raw != BRIDGE_JUNK; }
#endif

#if HAVE_MUSIC
/* Field 1..5 -> a volume.  Field 0 means "nobody chose", which is why interact.json's options are anchored at 1:
 * 0 is a legal volume (mute), and a menu that has never been touched must not silently mute the disc. */
static int music_vol_from_bridge(unsigned word) {
    switch ((word >> BRIDGE_MUSIC_SHIFT) & BRIDGE_MUSIC_MASK) {
    case 1: return   1;                       /* Off */
    case 2: return  64;
    case 3: return 128;
    case 4: return 192;
    case 5: return 255;
    default: return 0;                        /* 0, 6 or 7: not a value interact.json can produce */
    }
}
#endif  /* HAVE_MUSIC */

#if HAVE_FIT
/* Screen Fit: Letterbox / Stretch / Scaler are field values 1 / 2 / 3; the host's fit is 0 / 1 / 2.  Field 0 (or
 * 3, which 2 bits cannot hold beyond 1..3) means "no instruction" and night.ini's FIT keeps its value. */
static int fit_from_bridge(unsigned word) {
    switch ((word >> BRIDGE_FIT_SHIFT) & BRIDGE_FIT_MASK) {
    case 1: return 0;                         /* Letterbox */
    case 2: return 1;                         /* Stretch   */
    case 3: return 2;                         /* Scaler    */
    default: return -1;
    }
}
#endif  /* HAVE_FIT */

static int      g_fit_bridge = 0, g_interact_err = 0;
static int      g_diag = 0;          /* night.ini DIAG=: which experimental probes this run may perform */
static unsigned g_bridge_word = 0;   /* the raw control word last seen, and the source of every field above */
static int      g_disc_prefill_first = 150; /* night.ini DISC_PREFILL_FIRST: synchronous prefill slice (docs/376) */
static int      g_disc_min_want = 12;  /* night.ini DISC_MIN_WANT: sectors of allowance to accumulate before a fill read (docs/375) */
static int      g_disc_batch = 12;
static int      g_disc_lookahead = 240;   /* night.ini DISC_BATCH=: sectors per CD-DA slot read (docs/375) */
static int      g_pump_audio_tick = 0;   /* night.ini PUMP_AUDIO_TICK: 0 = drain the board ring per pump entry
                                           * (every build has always done this), 1 = once per 490 Hz tick */
static int      g_blit_skip = 0, g_sync_flip = 1, g_fb_mode = 1, g_fit = 1, g_blit_dirty = 0;
static int      g_present_skip = 1;
static int      g_row_skip = 0;
static int      g_present_testless = 24;   /* docs/456: frames to present without the full-surface change test once four consecutive frames have moved; 0 = the exact old shape */
static int      g_stall_ms = 400;
static int      g_fade_log = 0;
static int      g_fade_ramp = 0;
static int      g_fade_ramp_ms = 700;
static int      g_present_cap_ms = 0;  /* display-path cadence cap; 0 = present every poll (docs/441) */  /* wall time a palette ramp takes, whatever the frame rate (docs/439) */  /* play DAC fades back on the panel over their own step count (docs/436) */  /* log a witness line when no present has happened for this long (docs/433) */   /* 1 = write only the rows this display buffer does not already hold (docs/430) */
static int      g_palette_after_flip = 0;
static int      g_disc_idle_hook = 0;   /* 1 = present frames while a CD read waits on its DMA (docs/426) */
static int      g_fdump = 0;   /* 0 off, 1 L1+Up, 2 auto-arm at stage entry (docs/424) */   /* hardware-proven: 41.0 s -> 6.0 s logo hold, no visual defect (docs/422) */
static int      g_delay_us = 0;
static int      g_cdda_stop_fix = 1;  /* 1 = keep the invented stop at fn_000405a7 (INT 10h AX=0013, docs/413); 0 = do not stop CD-DA there, which is what the original can do */     /* 0 = leave f.c's g_delay_unit_us (500) alone. The original's own per-clock-read cost measures 1.27 ms on the card hold and 1.89 ms on the logo hold (docs/417), so a device that copies less per frame can also afford to wait longer per read. */   /* 1 = a full-surface hash decides whether the copy runs; the sampled 64-byte test below it can miss a small move (docs/416) */
static int      g_disc_async = 1;   /* 1 = feed the ring with double-buffered async reads (docs/403); 0 = the synchronous read shape */
static int      g_cdda_card_stop = 1;   /* 1 = the intro track stops when stage<N>.pcx is drawn: the original is silent for the card hold, and the stage track arrives on the game's own request (docs/413) */
static int      g_disc_ramp = 0;   /* 0 = prime the CD-DA slice in one visit (dry=0, audio_low=0); 1 = spread it over pump visits, which underran (docs/402) */
static int      g_screen    = 0;   /* 0 game only, 1 terminal only, 2 game with the log overlaid */

int of_setting_fit_cycle(void) {
    g_fit = (g_fit + 1) % 3;
    return g_fit;
}
/* ---- settings -------------------------------------------------------------------------
 * The instance's slot 2 is night.ini, which the chip32 loader parses as os.ini; of_config_get() reads
 * its sections.  On the desktop build the same keys come from the environment so a rig run can set them. */
static int cfg_int(const char *key, int dflt, const char *envkey) {
#ifdef OF_PC
    const char *e = getenv(envkey);
    return e ? atoi(e) : dflt;
#else
    /* night.ini writes its [night] keys upper case; the OS's key lookup is exact, so both spellings are
     * tried rather than guessing which one the parser normalises. */
    char buf[32], up[32];
    if (of_config_get("night", key, buf, sizeof buf) == 0) return atoi(buf);
    size_t i = 0;
    for (; key[i] && i < sizeof up - 1; i++) up[i] = (char)((key[i] >= 'a' && key[i] <= 'z') ? key[i] - 32 : key[i]);
    up[i] = 0;
    if (of_config_get("night", up, buf, sizeof buf) == 0) return atoi(buf);
    return dflt;
#endif
}

/* File logging is opt-in (docs/362).  The card log did its job -- it is how the peripheral mirror was found, how the
 * claim rule was fixed, and how packing was confirmed -- but it is a quarter-megabyte file in the player's Saves
 * folder that is written every eight lines, and a player never reads it.  night.ini LOG=1 brings it back, and every
 * diagnostic this host has is gated behind it (DIAG is a separate switch and ships at 0).  The desktop build keeps
 * logging on: the rig and the fixture suite read scratch/pocket/night.log as their witness. */
int of_setting_log_wanted(void) {
#ifdef OF_PC
    return cfg_int("log", 1, "NIGHT_LOG");
#else
    return cfg_int("log", 0, "NIGHT_LOG");
#endif
}

static int clamp_gain(int v) {                  /* 100 = unity; of_gain16's curve is clamping-free to 212 */
    if (v < 100) return 100;
    if (v > 212) return 212;
    return v;
}

static int clamp_vol(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void of_settings_load(void) {
    g_music_vol = clamp_vol(cfg_int("music_vol", 255, "NIGHT_MUSIC_VOL"));
    g_sfx_vol   = clamp_vol(cfg_int("sfx_vol",   255, "NIGHT_SFX_VOL"));
    g_master_vol = clamp_vol(cfg_int("master_vol", 255, "NIGHT_MASTER_VOL"));
    /* Sample gain, mixer volumes aside.  The PC build defaults to unity: the differential rig compares the port's
     * audio against the shipped disc byte for byte (docs/332) and against the captured board stream (docs/164), and a
     * gain stage in that path would make every one of those diffs a lie about the port's faithfulness. */
#ifdef OF_PC
    g_sfx_gain  = clamp_gain(cfg_int("sfx_gain", 100, "NIGHT_SFX_GAIN"));
    g_cdda_gain = clamp_gain(cfg_int("cdda_gain", 100, "NIGHT_CDDA_GAIN"));
#else
    g_sfx_gain  = clamp_gain(cfg_int("sfx_gain", 200, "NIGHT_SFX_GAIN"));
    g_cdda_gain = clamp_gain(cfg_int("cdda_gain", 150, "NIGHT_CDDA_GAIN"));
#endif
    /* Both spellings are accepted, and the explicit one is what the card gets now.  A key called CDDA whose
     * value 0 means "not off" reads like a mute switch and then surprises whoever edits the ini on the card:
     * CDDA_OFF=0 / PAD_OFF=0 say what they mean, and the old short keys still work for old cards. */
    g_cdda_on   = cfg_int("cdda_off", cfg_int("cdda", 0, "NIGHT_CDDA_OFF"), "NIGHT_CDDA_OFF") == 0;
    g_pad_on    = cfg_int("pad_off",  cfg_int("pad",  0, "NIGHT_PAD_OFF"),  "NIGHT_PAD_OFF")  == 0;
    /* night.ini decides by default; SCREEN=-1 hands the switch to the menu's Pocket LCD value.  A card the
     * owner edited is a deliberate instruction, and the interact bridge has already proved itself ambiguous
     * (five variables share 0xF7000000, and 0 is both "On" and "the bridge carried nothing"). */
    g_screen    = cfg_int("screen",      0, "NIGHT_SCREEN");   /* 0 game, 1 terminal, 2 overlay, -1 follow menu */
    /* Experimental reads are opt-IN, default off, and editable on the card without a rebuild.  0.1.35 shipped
     * four unproven probes in the boot path unconditionally (a 96-word MMIO scan of the periph page, an
     * of_analogizer_state() call, a full interact census with out-of-range probes, and a raw read of
     * 0xF7000000) and the run died just after the last of them, leaving the panel on openfpgaOS's terminal.  A
     * diagnostic must never be able to end a game: each one is a bit here, so a suspected probe is bisected by
     * editing night.ini, and a player's build does no bus reads at all. */
    g_diag      = cfg_int("diag",        0, "NIGHT_DIAG");
    g_tick_hz   = cfg_int("tick_hz",     0, "NIGHT_TICK_HZ");  /* 0 = whatever rate the game programs */
    /* Two knobs for the two hardware faults under investigation, each off/on from the card so a run changes
     * one thing at a time: BLIT_SKIP=0 stops the host skipping the blit when its sampled bytes say the frame
     * is unchanged (a stale-present theory for the ghosting), PRESENT_SYNC=0 stops waiting for the flip to be
     * presented (a tearing theory).  Both default to the cautious setting. */
    /* Off by default.  The sample-based test can miss a change (32 points in 307 200 bytes), and a missed
     * change holds the previous picture for up to eight presents -- which reads as the picture drifting
     * forward and back.  Now that a 320x200 present costs 1.8 ms median there is nothing left for the skip
     * to buy in the places it was doing the damage; BLIT_SKIP=1 re-arms it for performance experiments. */
    g_fb_mode = cfg_int("fb_mode",       1, "NIGHT_FB_MODE");   /* 1 = ask the OS for a 640x480 source FB */
    /* task-9 (docs/369): the board drain runs on EVERY pump entry, and the pump is entered ~267 000 times a
     * frame.  Gating it to the game's tick is the obvious lever and the risky one at once -- it empties the byte
     * ring the game's own DMA transfers write into -- so it ships off and a hardware run A/Bs it with one line. */
    g_pump_audio_tick = cfg_int("pump_audio_tick", 0, "NIGHT_PUMP_AUDIO_TICK");
    /* RESTORE=0 restores nothing (what every build before 0.1.61 did); 1 restores the HP bars, 2 the state bytes and
     * DAT_63374..63384, 4 the key table.  The state half is the one docs/334 measured as able to make the game
     * dereference out of range mid-stage, so it stays a developer's setting. */
    g_disc_batch = cfg_int("disc_batch", 12, "NIGHT_DISC_BATCH");
    g_disc_lookahead = cfg_int("disc_lookahead", 120, "NIGHT_DISC_LOOKAHEAD");   /* DEFAULT 120 = the value every desktop rig run was made with, so the shim basis is
                                                                                   * unchanged when no ini is present; the CARD gets 240 from night.ini (docs/407) */
    g_disc_min_want = cfg_int("disc_min_want", 12, "NIGHT_DISC_MIN_WANT");
    g_disc_prefill_first = cfg_int("disc_prefill_first", 150, "NIGHT_DISC_PREFILL_FIRST");
    /* How the 320x200 game picture meets the 320x240 panel.  1 (the default) fills the panel: 40 of the 200
     * rows are shown twice, one every fifth row, which is the evenest row-doubling stretch there is -- and
     * 320x200 on a 4:3 CRT *was* a 1.2x vertical stretch, so this reproduces the proportions the original was
     * seen in rather than the 20+20 black rows the port used to add.  0 puts the bars back.  2 is the
     * experiment: ask the core's own scaler for its declared 320x200 framebuffer mode and let the FPGA do the
     * stretch for free -- worth measuring, with docs/338 in mind (a mode the scaler cannot map shows black). */
    g_fit = cfg_int("fit", 1, "NIGHT_FIT");
    g_blit_skip = cfg_int("blit_skip",   0, "NIGHT_BLIT_SKIP");
    g_present_skip = cfg_int("present_skip", 1, "NIGHT_PRESENT_SKIP");
    g_row_skip = cfg_int("row_skip", 0, "NIGHT_ROW_SKIP");
    g_present_testless = cfg_int("present_testless", 24, "NIGHT_PRESENT_TESTLESS");
    g_stall_ms = cfg_int("stall_ms", 400, "NIGHT_STALL_MS");
    g_fade_log = cfg_int("fade_log", 0, "NIGHT_FADELOG");
    g_fade_ramp = cfg_int("fade_ramp", 0, "NIGHT_FADE_RAMP");
    g_fade_ramp_ms = cfg_int("fade_ramp_ms", 700, "NIGHT_FADE_RAMP_MS");
    g_present_cap_ms = cfg_int("present_cap_ms", 0, "NIGHT_PRESENT_CAP_MS");
    g_palette_after_flip = cfg_int("palette_after_flip", 0, "NIGHT_PALETTE_AFTER_FLIP");
    g_fdump = cfg_int("fdump", 0, "NIGHT_FDUMP");
    g_disc_idle_hook = cfg_int("disc_idle_hook", 0, "NIGHT_DISC_IDLE_HOOK");
    g_delay_us = cfg_int("delay_us", 0, "NIGHT_DELAY_US");
    g_cdda_stop_fix = cfg_int("cdda_stop_fix", 1, "NIGHT_CDDA_STOP_FIX");
    g_blit_dirty = cfg_int("blit_dirty",  1, "NIGHT_BLIT_DIRTY");
    g_disc_ramp = cfg_int("disc_ramp",   0, "NIGHT_DISC_RAMP");
    g_cdda_card_stop = cfg_int("cdda_card_stop", 1, "NIGHT_CDDA_CARD_STOP");
    g_disc_async = cfg_int("disc_async",  1, "NIGHT_DISC_ASYNC");
    g_sync_flip = cfg_int("present_sync", 1, "NIGHT_PRESENT_SYNC");
#ifndef OF_PC
    /* of_interact_get() indexes by position in interact.json; the reserved SDK entries now occupy the first
     * six, so the indices come from the header mk_dist.py generates alongside the JSON.
     *
     * A menu value is only accepted when it is one of the values interact.json offers (MUSIC_OPTS).  An
     * untouched variable reads 0 on the wire, and 0 is a legal volume -- mute -- so "nobody moved it" and
     * "the user asked for silence" would otherwise be the same number, and the first silently replaced
     * night.ini's MUSIC_VOL (docs/341, and the card's own interact_persist.json showing Music Volume 0). */
    {
        unsigned w = 0;                       /* there is no bridge word to read; see bridge_control_word's epitaph */
        int mv = 0, fit = -1;
        SLOG("bridge: no control-word read (fatal on this core, docs/349).  What the documented ecall says about\n"
             "       the one menu entry the core really answers:\n");
        unsigned iv = 0;
        long e = of_interact_probe(OF_IDX_PLCD, &iv);
        SLOG("bridge: lcd index %d err=%ld val=%08x\n", OF_IDX_PLCD, e, iv);
        int lcd = 0;
        g_lcd_raw = (unsigned)lcd;
        if (mv) g_music_vol = clamp_vol(mv);
        if (fit >= 0) g_fit = fit;
        /* SCREEN=-1 is left UNRESOLVED now.  It used to be answered here from the bridge, which is either fatal
         * to read (0xF7000000) or empty (the ecall), so resolving it at load time silently turned -1 into 0.  The
         * follow happens per second in main_of.c from periph register 0x40000080, where 0.1.38 saw On/Off/Terminal
         * actually land; with the periph probe off it behaves as 0. */
        if (g_screen < 0)
            SLOG("screen: -1 follows the core's Pocket LCD register 0x40000080 (needs DIAG bit 0); until a"
                 " reading arrives it behaves as 0\n");
        SLOG("bridge decoded: music=%d fit=%d lcd=%08x\n", mv, fit, (unsigned)lcd);
    }
#endif

    printf("settings: music_vol=%d sfx_vol=%d cdda=%d pad=%d tick_hz=%d screen=%d\n",
           g_music_vol, g_sfx_vol, g_cdda_on, g_pad_on, g_tick_hz, g_screen);
}

int of_settings_poll(void) {
    /* Nothing to poll.  Every setting this host applies comes from night.ini, read once at load; the only live
     * settings source the port ever had was the control word, and reading it hangs the core (docs/349).  The
     * interact ecall is not a substitute -- three builds of card logs showed it returning a fixed table of junk
     * for every index, including the index whose menu entry demonstrably moves the panel. */
    return 0;
}
#if 0
int of_settings_poll_disabled(void) {
    int changed = 0;
    unsigned w = 0;
    int mv = 0, fit = -1;
#if HAVE_MUSIC
    mv = music_vol_from_bridge(w);
#endif
#if HAVE_FIT
    fit = fit_from_bridge(w);
#endif
    unsigned lcd = w & 0x00030000u;

    if (w != g_bridge_word) {
        g_bridge_word = w;
        SLOG("bridge word=%08x music=%d fit=%d lcd=%02x\n", w, mv, fit, (unsigned)(lcd >> PLCD_SHIFT));
#if HAVE_MUSIC
        { unsigned iv = 0; long e = of_interact_probe(OF_IDX_MUSIC, &iv);
          SLOG("  cross-check interact[%d]: err=%ld val=%08x\n", OF_IDX_MUSIC, e, iv); }
#endif
#if HAVE_FIT
        { unsigned iv = 0; long e = of_interact_probe(OF_IDX_FIT, &iv);
          SLOG("  cross-check interact[%d]: err=%ld val=%08x\n", OF_IDX_FIT, e, iv); }
#endif
        { unsigned iv = 0; long e = of_interact_probe(OF_IDX_PLCD, &iv);
          SLOG("  cross-check interact[%d]: err=%ld val=%08x\n", OF_IDX_PLCD, e, iv); }
        changed |= OF_SET_VOL_SEEN | OF_SET_FIT_SEEN;
    }

    if (mv && mv != g_music_vol) {
        g_music_vol = clamp_vol(mv);
        changed |= OF_SET_VOL;
    }
    if (fit >= 0 && fit != g_fit) { g_fit = fit; changed |= OF_SET_FIT; }
    /* No menu knob for the tick cap (docs/341): night.ini TICK_HZ, a developer's instrument. */
    if (lcd != g_lcd_raw) {
        g_lcd_raw = lcd;
        changed |= OF_SET_LCD;
        if (g_screen < 0) g_screen = (lcd == OF_PLCD_TERMINAL) ? 1 : 0;
    }
    return changed;
}
#endif

int of_setting_apply_music_vol(int v) {   /* a value that arrived from a peripheral slot, not from night.ini */
    v = clamp_vol(v);
    if (v == g_music_vol) return 0;
    g_music_vol = v;
    return OF_SET_VOL;
}
int of_setting_apply_sfx_vol(int v) {     /* the sound-effect group, which the game itself never attenuates */
    v = clamp_vol(v);
    if (v == g_sfx_vol) return 0;
    g_sfx_vol = v;
    return OF_SET_SFX;
}
int of_setting_apply_fit(int f) {         /* 0 letterbox, 1 stretch, 2 scaler -- the same three the pad cycles */
    if (f < 0 || f > 2 || f == g_fit) return 0;
    g_fit = f;
    return OF_SET_FIT;
}
int of_setting_music_vol(void) { return g_music_vol; }
int of_setting_sfx_vol(void)   { return g_sfx_vol; }
int of_setting_master_vol(void)  { return g_master_vol; }
int of_setting_sfx_gain(void)  { return g_sfx_gain; }
int of_setting_cdda_gain(void) { return g_cdda_gain; }
int of_setting_cdda(void)      { return g_cdda_on; }
int of_setting_pad(void)       { return g_pad_on; }
int of_setting_tick_hz(void)   { return g_tick_hz; }
int of_setting_music_bridge(void) { return g_music_bridge; }   /* raw word, for the log */
int of_setting_fit_bridge(void)   { return g_fit_bridge; }
int of_setting_interact_err(void) { return g_interact_err; }
int of_setting_fit(void)       { return g_fit; }      /* 0 letterbox, 1 stretch, 2 scaler 320x200 */
int of_setting_screen(void)    { return g_screen; }
int  of_setting_blit_skip(void) { return g_blit_skip; }
int  of_setting_present_skip(void) { return g_present_skip; }
int  of_setting_row_skip(void) { return g_row_skip; }
int  of_setting_present_testless(void) { return g_present_testless; }
int  of_setting_stall_ms(void) { return g_stall_ms; }
int  of_setting_fade_log(void) { return g_fade_log; }
int  of_setting_fade_ramp(void) { return g_fade_ramp; }
int  of_setting_fade_ramp_ms(void) { return g_fade_ramp_ms; }
int  of_setting_present_cap_ms(void) { return g_present_cap_ms; }
int  of_setting_palette_after_flip(void) { return g_palette_after_flip; }
int  of_setting_fdump(void) { return g_fdump; }
int  of_setting_disc_idle_hook(void) { return g_disc_idle_hook; }
int  of_setting_delay_us(void) { return g_delay_us; }
int  of_setting_cdda_stop_fix(void) { return g_cdda_stop_fix; }
int  of_setting_blit_dirty(void) { return g_blit_dirty; }
int  of_setting_disc_ramp(void)  { return g_disc_ramp; }
int  of_setting_cdda_card_stop(void) { return g_cdda_card_stop; }
int  of_setting_disc_async(void) { return g_disc_async; }
int  of_setting_sync_flip(void) { return g_sync_flip; }
int  of_setting_pump_audio_tick(void) { return g_pump_audio_tick; }
int  of_setting_disc_batch(void)  { return g_disc_batch; }
int  of_setting_disc_lookahead(void) { return g_disc_lookahead; }
int  of_setting_disc_min_want(void){ return g_disc_min_want; }
int  of_setting_disc_prefill_first(void){ return g_disc_prefill_first; }        /* save-apply bits: 1 gauges (shipping), 2 state, 4 key table */
int  of_setting_fb_mode(void)   { return g_fb_mode; }     /* 1 = use a native 640x480 source framebuffer if the OS offers one */
int  of_setting_lcd_raw(void) { return (int)g_lcd_raw; }
int  of_setting_bridge_word(void) { return (int)g_bridge_word; }
int  of_setting_diag(void)      { return g_diag; }   /* the Pocket LCD field only, not the shared word */

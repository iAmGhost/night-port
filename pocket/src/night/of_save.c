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
#include <stdlib.h>
#include <string.h>

#ifndef OF_PC
#include "of.h"
#include "of_config.h"
#include "of_interact_idx.h"        /* generated: which position in interact.json each knob sits at */
#include <dirent.h>
#endif

#define SAVE_MAGIC   0x5653474eu   /* 'NGSV' */
#define SAVE_VERSION 1u
#define SAVE_STAGE_FIRST 10u       /* instance slot ids 10..16 = night_0.sav .. night_6.sav */
/* The instance filename a save slot is bound to -- the name that has to reach of_file_slot_register() too. */
static const char *save_fname(int slot, char *out, size_t n) { snprintf(out, n, "night_%d.sav", slot); return out; }
#define SAVE_FILE(slot)  save_fname((slot), savebuf, sizeof savebuf)
/* Only the scalar window of the player block is saved.  The block's head holds host pointers (sprite and
 * action-table addresses into the heap arena), and a save file must not carry a pointer from one process's
 * arena into another: the first version copied 64 raw bytes and the restore died in a SIGBUS the moment the
 * game dereferenced one.  The window below is exactly the set of fields f.h documents as scalars -- the two
 * gauge words at +0x23/+0x29 (fn_01348f draws them) and the state bytes +0x25..+0x2b that DAT_63374..63384
 * mirror. */
#define PLAYER_WIN_OFF 0x20
#define PLAYER_WIN_LEN 0x10
#define PLAYER_BYTES   PLAYER_WIN_LEN

typedef struct {
    uint32_t magic, version;
    int64_t  created_ms;
    uint32_t stage;
    uint32_t dat63370, dat63374, dat63378, dat6337c;
    uint8_t  dat63380, dat63384;
    uint8_t  player[PLAYER_BYTES];
    uint8_t  key_state[0x80];      /* the DAT_00063e88 held-byte table, exactly as the game holds it */
    uint32_t stage_live_calls;     /* fn_03272f invocations at save time: the proof the stage was live */
    uint32_t crc;
} save_blob_t;

static save_blob_t g_pending;
static int      g_pending_valid;
static int      g_slot = 1;
static char     g_err[96];
static int      g_music_vol = 255, g_sfx_vol = 255, g_cdda_on = 1, g_pad_on = 1;
static int      g_tick_hz   = 0;   /* 0 = whatever PIT rate the game programs; see of_setting_tick_hz */
static unsigned g_lcd_raw   = 0xDEADBEEFu;   /* 0xDEADBEEU until the bridge is read */
/* The values interact.json actually offers (mk_dist.py writes the same lists).  A menu variable is a word in
 * the interact bridge, and at boot the bridge can carry 0 for a variable nobody has touched -- 0 is a legal
 * volume (mute) and a legal "no cap", so the host only accepts a value it can name.  That is why the two
 * knobs are lists with named options and no 0 in them: an unwired 0 must not silently mute the music. */
static const int MUSIC_OPTS[] = {   1,   64,  128,  192,  255 };   /* Off, Low, Medium, High, Full */
static const int TICK_OPTS[]  = {   0,   60,  122,  245,  490 };   /* Game rate, 1/8, 1/4, 1/2, full */
static int in_opts(const int *t, int n, int v) { for (int i = 0; i < n; i++) if (t[i] == v) return 1; return 0; }
static int      g_blit_skip = 0, g_sync_flip = 1, g_fb_mode = 1;
static int      g_screen    = 0;   /* 0 game only, 1 terminal only, 2 game with the log overlaid */

/* The blob stores the player window (PLAYER_WIN_OFF..+0x10), so its gauge words sit at 3 and 9. */
static int16_t gauge_of(const save_blob_t *b)  { return (int16_t)(b->player[0x23 - PLAYER_WIN_OFF] | (b->player[0x24 - PLAYER_WIN_OFF] << 8)); }
static int16_t gauge2_of(const save_blob_t *b) { return (int16_t)(b->player[0x29 - PLAYER_WIN_OFF] | (b->player[0x2a - PLAYER_WIN_OFF] << 8)); }

/* What may be written back into a LIVE stage.  Measured (docs/334): the action-state bytes 0x25..0x28 are
 * the selectors the stage's key/action tables index through (DAT_63378/6337c are picked from them by
 * fn_0376c8), and forcing them mid-stage makes the game dereference out of range -- a bus error, not a
 * graceful mismatch.  The gauges are plain int16 bars that fn_01348f only draws, and those restore fine.
 * NIGHT_RESTORE=<bits> bisects it: 1 = gauges, 2 = state bytes, 4 = the DAT_63e88 key table. */
#define RS_GAUGE    1
#define RS_STATE    2
#define RS_KEYSTATE 4
static int restore_mask(void) {
    const char *e = getenv("NIGHT_RESTORE");
    return e ? (atoi(e) & 7) : 0;                 /* 0 = the stage checkpoint only, the measured-safe default */
}

static uint32_t blob_crc(const save_blob_t *b) {
    const uint8_t *p = (const uint8_t *)b;
    uint32_t c = 0x811c9dc5u;
    for (size_t i = 0; i < offsetof(save_blob_t, crc); i++) {
        c ^= p[i];
        c = (c * 0x01000193u) & 0xffffffffu;
    }
    return c;
}

/* ---- slot resolution ------------------------------------------------------------------ */
#ifdef OF_PC
static const char *save_dir(void) {
    const char *d = getenv("NIGHT_SAVE_DIR");
    return (d && *d) ? d : "scratch/pocket/saves";
}
static int slot_path(int slot, char *out, size_t n) {
    if (slot < 0 || slot >= OF_SAVE_SLOTS) { snprintf(g_err, sizeof g_err, "slot %d out of range", slot); return -1; }
    snprintf(out, n, "%s/night_%d.sav", save_dir(), slot);
    return 0;
}
#else
/* Bound slots are flat in the virtual root and only identifiable through d_ino. */
static int slot_path(int slot, char *out, size_t n) {
    if (slot < 0 || slot >= OF_SAVE_SLOTS) { snprintf(g_err, sizeof g_err, "slot %d out of range", slot); return -1; }
    uint32_t want = SAVE_STAGE_FIRST + (uint32_t)slot;
    char want_name[32];
    snprintf(want_name, sizeof want_name, "night_%d.sav", slot);
    DIR *d = opendir("/");
    if (!d) { snprintf(g_err, sizeof g_err, "opendir(/) failed"); return -1; }
    struct dirent *e;
    int found = 0;
    while ((e = readdir(d)) != NULL) {
        if ((uint32_t)e->d_ino - 1u == want) { snprintf(out, n, "%s", e->d_name); found = 1; break; }
    }
    closedir(d);
    if (!found) {
        /* A nonvolatile slot whose file does not exist yet is not listed by readdir at all -- the firmware
         * keeps save data under Saves/<platform>/common/ (measured on this card: quake.cfg lives at
         * Saves/quake/common/, and Saves/night/common/ was created for us but left empty).  openfpgaOS-Quake
         * writes its cfg/sav slots with a plain fopen on the BARE name, which the SDK maps to the bound slot,
         * so that is the fallback rather than declaring the slot unreachable (docs/336). */
        snprintf(out, n, "%s", want_name);
        return 0;
    }
    return 0;
}
#endif

/* ---- capture -------------------------------------------------------------------------- */
static void blob_capture(save_blob_t *b) {
    memset(b, 0, sizeof *b);
    b->magic = SAVE_MAGIC;
    b->version = SAVE_VERSION;
    b->created_ms = (int64_t)(sys_now_us() / 1000ull);
    b->stage = g_dat_63370 ? g_dat_63370 : (uint32_t)(g_warp_stage > 0 ? (uint32_t)g_warp_stage : 1u);
    b->dat63370 = g_dat_63370;
    b->dat63374 = g_dat_63374;
    b->dat63378 = g_dat_63378;
    b->dat6337c = g_dat_6337c;
    b->dat63380 = g_dat_63380;
    b->dat63384 = g_dat_63384;
    if (g_player_ent) memcpy(b->player, g_player_ent + PLAYER_WIN_OFF, PLAYER_BYTES);
    memcpy(b->key_state, g_dat_63e88, sizeof b->key_state);
    b->stage_live_calls = (uint32_t)f_act_resolver_calls();
    b->crc = blob_crc(b);
    if (g_player_ent) {                           /* what the record head actually holds, for the record */
        printf("save: player record 0x00..0x3f:");
        for (int i = 0; i < 0x40; i++) printf(" %02x", g_player_ent[i]);
        printf("\n");
    }
}

int of_save_write(int slot) {
    char path[512];
    if (slot_path(slot, path, sizeof path) != 0) return -1;
#ifndef OF_PC
    /* No of_file_slot_register(): openfpgaOS-Quake writes Saves/quake/common/s0.sav with a plain
     * fopen("s0.sav","wb") and no registration, and of_file.h describes a registered binding as "a read-only
     * APF data file".  Our registered names wrote nothing across five hardware runs. */
    char savebuf[32];
    (void)SAVE_FILE(slot);          /* the bare name is what the kernel's discovery maps to the slot */
#endif
    save_blob_t b;
    blob_capture(&b);
    FILE *f = fopen(path, "wb");
    if (!f) { snprintf(g_err, sizeof g_err, "open %s failed", path); return -1; }
    size_t wrote = fwrite(&b, 1, sizeof b, f);
    fclose(f);
    if (wrote != sizeof b) { snprintf(g_err, sizeof g_err, "wrote %zu of %zu bytes", wrote, sizeof b); return -1; }
    printf("save: slot %d -> %s (%zu B) stage=%u gauge=%d/%d state=%02x/%02x/%02x resolver_calls=%u\n",
           slot, path, sizeof b, b.dat63370, gauge_of(&b), gauge2_of(&b),
           b.player[0x25 - PLAYER_WIN_OFF], b.player[0x26 - PLAYER_WIN_OFF], b.player[0x27 - PLAYER_WIN_OFF],
           b.stage_live_calls);
    return 0;
}

int of_save_read(int slot) {
    char path[512];
    if (slot_path(slot, path, sizeof path) != 0) return -1;
#ifndef OF_PC
    char savebuf[32];
    (void)SAVE_FILE(slot);
#endif
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(g_err, sizeof g_err, "no save in slot %d (%s)", slot, path); return -1; }
    save_blob_t b;
    size_t got = fread(&b, 1, sizeof b, f);
    fclose(f);
    if (got != sizeof b) { snprintf(g_err, sizeof g_err, "slot %d holds %zu of %zu bytes", slot, got, sizeof b); return -1; }
    if (b.magic != SAVE_MAGIC || b.version != SAVE_VERSION) { snprintf(g_err, sizeof g_err, "slot %d is not a NIGHT save", slot); return -1; }
    if (b.crc != blob_crc(&b)) { snprintf(g_err, sizeof g_err, "slot %d checksum mismatch", slot); return -1; }
    g_pending = b;
    g_pending_valid = 1;
    g_warp_stage = (int)b.stage;                    /* the warp basis the restore drives (docs/52) */
    printf("load: slot %d <- %s stage=%u gauge=%d/%d state=%02x/%02x/%02x saved_at_ms=%lld\n",
           slot, path, b.dat63370, gauge_of(&b), gauge2_of(&b),
           b.player[0x25 - PLAYER_WIN_OFF], b.player[0x26 - PLAYER_WIN_OFF], b.player[0x27 - PLAYER_WIN_OFF],
           (long long)b.created_ms);
    return 0;
}

int of_save_apply(void) {
    if (!g_pending_valid) return 0;
    if (!g_player_ent) return 0;                    /* wait until the stage has spawned the player */
    int mask = restore_mask();
    if (mask & RS_GAUGE) {
        g_player_ent[0x23] = g_pending.player[0x23 - PLAYER_WIN_OFF];
        g_player_ent[0x24] = g_pending.player[0x24 - PLAYER_WIN_OFF];
        g_player_ent[0x29] = g_pending.player[0x29 - PLAYER_WIN_OFF];
        g_player_ent[0x2a] = g_pending.player[0x2a - PLAYER_WIN_OFF];
    }
    if (mask & RS_STATE) {
        for (int off = 0x25; off <= 0x28; off++) g_player_ent[off] = g_pending.player[off - PLAYER_WIN_OFF];
        g_dat_63374 = g_pending.dat63374;
        g_dat_63378 = g_pending.dat63378;
        g_dat_6337c = g_pending.dat6337c;
        g_dat_63380 = g_pending.dat63380;
        g_dat_63384 = g_pending.dat63384;
    }
    if (mask & RS_KEYSTATE) memcpy(g_dat_63e88, g_pending.key_state, sizeof g_pending.key_state);
    printf("load: restore mask=%d\n", mask);
    g_pending_valid = 0;
    printf("load: restored player record (stage=%u gauge=%d/%d state=%02x/%02x/%02x)\n",
           g_dat_63370,
           (int16_t)(g_player_ent[0x23] | (g_player_ent[0x24] << 8)),
           (int16_t)(g_player_ent[0x29] | (g_player_ent[0x2a] << 8)),
           g_player_ent[0x25], g_player_ent[0x26], g_player_ent[0x27]);
    return 1;
}

int         of_save_slot(void)     { return g_slot; }
void        of_save_slot_next(void){ g_slot = (g_slot + 1) % OF_SAVE_SLOTS; printf("save slot: %d\n", g_slot); }
int         of_save_pending(void)  { return g_pending_valid; }
const char *of_save_last_error(void){ return g_err[0] ? g_err : (const char *)0; }

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

static int clamp_vol(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void of_settings_load(void) {
    g_music_vol = clamp_vol(cfg_int("music_vol", 255, "NIGHT_MUSIC_VOL"));
    g_sfx_vol   = clamp_vol(cfg_int("sfx_vol",   255, "NIGHT_SFX_VOL"));
    /* Both spellings are accepted, and the explicit one is what the card gets now.  A key called CDDA whose
     * value 0 means "not off" reads like a mute switch and then surprises whoever edits the ini on the card:
     * CDDA_OFF=0 / PAD_OFF=0 say what they mean, and the old short keys still work for old cards. */
    g_cdda_on   = cfg_int("cdda_off", cfg_int("cdda", 0, "NIGHT_CDDA_OFF"), "NIGHT_CDDA_OFF") == 0;
    g_pad_on    = cfg_int("pad_off",  cfg_int("pad",  0, "NIGHT_PAD_OFF"),  "NIGHT_PAD_OFF")  == 0;
    /* night.ini decides by default; SCREEN=-1 hands the switch to the menu's Pocket LCD value.  A card the
     * owner edited is a deliberate instruction, and the interact bridge has already proved itself ambiguous
     * (five variables share 0xF7000000, and 0 is both "On" and "the bridge carried nothing"). */
    g_screen    = cfg_int("screen",      0, "NIGHT_SCREEN");   /* 0 game, 1 terminal, 2 overlay, -1 follow menu */
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
    g_blit_skip = cfg_int("blit_skip",   0, "NIGHT_BLIT_SKIP");
    g_sync_flip = cfg_int("present_sync", 1, "NIGHT_PRESENT_SYNC");
#ifndef OF_PC
    /* of_interact_get() indexes by position in interact.json; the reserved SDK entries now occupy the first
     * six, so the indices come from the header mk_dist.py generates alongside the JSON.
     *
     * A menu value is only applied when it differs from its defaultval.  That distinguishes "the user moved
     * this slider" from "the bridge carries nothing" -- with no value wired through, of_interact_get() reads
     * 0, and 0 is a legal volume (mute) that would otherwise silently replace night.ini's MUSIC_VOL. */
    {
        int mv = (int)of_interact_get(OF_IDX_MUSIC);
        int tk = (int)of_interact_get(OF_IDX_TICK);
        /* The Pocket LCD list lives in bits 16-17 of 0xF7000000, and four other interact variables share that
         * same word (Analogizer, the two SNAC lists, Analogizer Video Out, Video H/V Offset).  of_interact_get
         * hands back a 32-bit value and the header never promises it is masked to the variable's own field, so
         * comparing the whole word against OF_PLCD_TERMINAL only ever matched if every other variable happened
         * to sit at zero -- which is how "Terminal" could be selected in the menu and never reach the host. */
        int lcd = (int)(of_interact_get(OF_IDX_PLCD) & 0x00030000u);
        g_lcd_raw = (unsigned)lcd;          /* the raw bridge word, reported every second */
        if (in_opts(MUSIC_OPTS, (int)(sizeof MUSIC_OPTS / sizeof *MUSIC_OPTS), mv)) g_music_vol = clamp_vol(mv);
        if (in_opts(TICK_OPTS,  (int)(sizeof TICK_OPTS  / sizeof *TICK_OPTS),  tk)) g_tick_hz = tk;
        /* "Pocket LCD" is the firmware's own screen switch (bits 16-17 of 0xF7000000) and the only one the
         * Core Options menu will edit.  It wins over night.ini when it carries a value; night.ini's SCREEN
         * stays as the route for a card-only diagnostic run (SCREEN=2 asks for the log overlaid on the game). */
        if (g_screen < 0) g_screen = (lcd == OF_PLCD_TERMINAL) ? 1 : 0;
    }
#endif

    printf("settings: music_vol=%d sfx_vol=%d cdda=%d pad=%d tick_hz=%d screen=%d\n",
           g_music_vol, g_sfx_vol, g_cdda_on, g_pad_on, g_tick_hz, g_screen);
}

int of_settings_poll(void) {
#ifdef OF_PC
    return 0;                       /* the desktop build has no interact bridge */
#else
    int changed = 0;
    int mv = (int)of_interact_get(OF_IDX_MUSIC);
    int tk = (int)of_interact_get(OF_IDX_TICK);
    unsigned lcd = (unsigned)(of_interact_get(OF_IDX_PLCD) & 0x00030000u);

    if (in_opts(MUSIC_OPTS, (int)(sizeof MUSIC_OPTS / sizeof *MUSIC_OPTS), mv) && mv != g_music_vol) {
        g_music_vol = clamp_vol(mv);
        changed |= OF_SET_VOL;
    }
    if (in_opts(TICK_OPTS, (int)(sizeof TICK_OPTS / sizeof *TICK_OPTS), tk) && tk != g_tick_hz) {
        g_tick_hz = tk;
        changed |= OF_SET_TICK;
    }
    if (lcd != g_lcd_raw) {
        g_lcd_raw = lcd;
        changed |= OF_SET_LCD;
        if (g_screen < 0) g_screen = (lcd == OF_PLCD_TERMINAL) ? 1 : 0;
    }
    return changed;
#endif
}

int of_setting_music_vol(void) { return g_music_vol; }
int of_setting_sfx_vol(void)   { return g_sfx_vol; }
int of_setting_cdda(void)      { return g_cdda_on; }
int of_setting_pad(void)       { return g_pad_on; }
int of_setting_tick_hz(void)   { return g_tick_hz; }
int of_setting_screen(void)    { return g_screen; }
int  of_setting_blit_skip(void) { return g_blit_skip; }
int  of_setting_sync_flip(void) { return g_sync_flip; }
int  of_setting_fb_mode(void)   { return g_fb_mode; }     /* 1 = use a native 640x480 source framebuffer if the OS offers one */
int  of_setting_lcd_raw(void) { return (int)g_lcd_raw; }   /* the Pocket LCD field only, not the shared word */

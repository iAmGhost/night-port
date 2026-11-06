/* of_save.h -- the Pocket host's save slots and settings (task-7). */
#ifndef NIGHT_OF_SAVE_H
#define NIGHT_OF_SAVE_H

#include <stdint.h>

#define OF_SAVE_SLOTS 7          /* data slot ids 10..16; id 17 is the diagnostic log (of_log.c) */

/* Resolve + open one of the instance's save slots (ids 10..19 -> night_N.sav).
 * Returns 0 on success.  On the desktop build the slots are ordinary files under
 * NIGHT_SAVE_DIR (default scratch/pocket/saves). */
int  of_save_write(int slot);
int  of_save_read(int slot);

/* Apply a loaded blob to the live game state.  Call once the stage is live; it reports whether it did. */
int  of_save_apply(void);
int  of_setting_restore(void);
int  of_setting_disc_prefill_first(void); /* night.ini DISC_PREFILL_FIRST: sectors read before the voices start (12..300) */
int  of_setting_disc_min_want(void); /* night.ini DISC_MIN_WANT: sectors of allowance to accumulate before a fill read (1..12) */
int  of_setting_disc_batch(void);    /* night.ini DISC_BATCH=: sectors per CD-DA slot read (1..12) */     /* night.ini RESTORE=: bits a load applies -- 1 gauges (default), 2 state+selectors, 4 key table */

int  of_save_slot(void);                 /* the slot the pad combo is currently addressing */
void of_save_slot_next(void);
int  of_save_pending(void);              /* a load is armed and not yet applied */
const char *of_save_last_error(void);

/* Settings: os.ini ([night] section) on the device, environment on the desktop. */
void of_settings_load(void);
/* Re-read the interact menu and apply whatever moved.  Returns a bitmask of what changed; the menu is live
 * during a run, so a knob has to be readable mid-game, not only at boot (docs/341). */
#define OF_SET_VOL  1
#define OF_SET_SFX  16
#define OF_SET_VOL_SEEN 8     /* the raw word moved, even if it decodes to nothing usable */
#define OF_SET_LCD  2
#define OF_SET_FIT  4         /* the Screen Fit knob moved (letterbox <-> stretch <-> scaler) */
#define OF_SET_FIT_SEEN 16    /* its raw word moved, usable or not */

/* Read one interact variable AND the ecall's error.  of_interact_get() throws the error away and returns only
 * .value, which is how 0.1.28's card log could show 0x55555555 at every index without saying whether the OS
 * answered that or refused the call. */
long of_interact_probe(int index, unsigned *value);
int  of_settings_poll(void);
int  of_setting_music_vol(void);
int  of_setting_apply_music_vol(int v);   /* returns OF_SET_VOL when it changed something */
int  of_setting_apply_fit(int f);
int  of_setting_apply_sfx_vol(int v);
int  of_setting_sfx_vol(void);
int  of_setting_master_vol(void);      /* the mixer output stage; 255 = no attenuation */
int  of_setting_log_wanted(void);    /* night.ini LOG=: open the card's nightlog.sav at all? */
int  of_setting_sfx_gain(void);        /* percent, 100..212, applied through of_gain16()'s knee */
int  of_setting_cdda_gain(void);
int  of_setting_cdda(void);
int  of_setting_pad(void);
int  of_setting_pump_audio_tick(void);  /* 0 (default) = drain per pump entry; 1 = per tick (task-9 A/B) */
int  of_setting_tick_hz(void);        /* 0 = the PIT rate the game programs */
int  of_setting_music_bridge(void);   /* the raw interact word behind Music Volume */
int  of_setting_fit(void);          /* 0 letterbox (default), 1 stretch 320x200 -> 320x240, 2 ask the scaler for 320x200 */
int  of_setting_fit_bridge(void);   /* the raw interact word behind Screen Fit, for the log */
int  of_setting_fit_cycle(void);    /* 0 -> 1 -> 2 -> 0 from the pad combo; returns the new fit */
int  of_setting_interact_err(void); /* the error code the last interact probe reported */
int  of_setting_screen(void);         /* 0 game, 1 terminal, 2 game + log overlay */
int  of_setting_lcd_raw(void);
int  of_setting_bridge_word(void);   /* last raw 0xF7000000 read, 0 when the probe is disabled */
int  of_setting_diag(void);          /* night.ini DIAG= bitmask, see the OF_DIAG_* below */

/* Which experimental reads a run is allowed to perform, all off unless night.ini says otherwise.  Every one of
 * these is a probe the port has NOT established as safe on this bitstream, and 0.1.35 is the evidence: shipped
 * with all of them unconditional, the run stopped writing its log after the last probe and the panel stayed on
 * the openfpgaOS terminal. */
#define OF_DIAG_PERIPH      1     /* MMIO scan of the openfpgaOS periph page at 0x40000000+ (interactdemo's route) */
#define OF_DIAG_ANALOGIZER  2     /* of_analogizer_state(), the documented route to the reserved menu values */
#define OF_DIAG_INTERACT    4     /* the interact census, in range only -- see docs/347 for the edge probes */
/* bit 3 (8) is RETIRED, not merely off: an app read of 0xF7000000 hangs this core.  0.1.37 ran with DIAG=15 and the
 * card log ends at "probe: control-word read begin" -- no "done", no game, a frozen pocket.  The value is accepted
 * in night.ini so an old card does nothing strange, and no code path consumes it. */
#define OF_DIAG_BRIDGE      8     /* retired: reading the control word is fatal here (docs/349) */
/* The boot-time file probe wrote 17 bytes into night_6.sav -- a *player save slot* -- on every boot, unconditionally.
 * The card proves it: the file reappeared at 14:09 on 2026-11-06 with "NIGHT-PROBE-0001" after being deleted (docs/394).
 * It is now opt-in and it no longer writes to any save slot. */
#define OF_DIAG_FILEPROBE   16    /* the read-only file-layer probe: slot find + readdir, no writes at all */        /* Pocket LCD field only (bits 16-17), 0xDEADBEEU before a read */
int  of_setting_blit_skip(void);      /* 1 = allow the host to skip a blit when the sampled frame is unchanged */
int  of_setting_sync_flip(void);      /* 1 = wait for each flip to be presented before returning */
int  of_setting_fb_mode(void);        /* 1 = ask the OS for a native 640x480 source framebuffer (FB_MODE=0 forces the software downscale) */

#endif /* NIGHT_OF_SAVE_H */

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

int  of_save_slot(void);                 /* the slot the pad combo is currently addressing */
void of_save_slot_next(void);
int  of_save_pending(void);              /* a load is armed and not yet applied */
const char *of_save_last_error(void);

/* Settings: os.ini ([night] section) on the device, environment on the desktop. */
void of_settings_load(void);
/* Re-read the interact menu and apply whatever moved.  Returns a bitmask of what changed; the menu is live
 * during a run, so a knob has to be readable mid-game, not only at boot (docs/341). */
#define OF_SET_VOL  1
#define OF_SET_TICK 2
#define OF_SET_LCD  4
int  of_settings_poll(void);
int  of_setting_music_vol(void);
int  of_setting_sfx_vol(void);
int  of_setting_cdda(void);
int  of_setting_pad(void);
int  of_setting_tick_hz(void);        /* 0 = the PIT rate the game programs */
int  of_setting_screen(void);         /* 0 game, 1 terminal, 2 game + log overlay */
int  of_setting_lcd_raw(void);        /* Pocket LCD field only (bits 16-17), 0xDEADBEEU before a read */
int  of_setting_blit_skip(void);      /* 1 = allow the host to skip a blit when the sampled frame is unchanged */
int  of_setting_sync_flip(void);      /* 1 = wait for each flip to be presented before returning */
int  of_setting_fb_mode(void);        /* 1 = ask the OS for a native 640x480 source framebuffer (FB_MODE=0 forces the software downscale) */

#endif /* NIGHT_OF_SAVE_H */

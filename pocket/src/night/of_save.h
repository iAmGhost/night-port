/* of_save.h -- the Pocket host's save slots and settings (task-7). */
#ifndef NIGHT_OF_SAVE_H
#define NIGHT_OF_SAVE_H

#include <stdint.h>

#define OF_SAVE_SLOTS 8          /* data slot ids 10..17; the manifest budget (docs/334 §7) */

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
int  of_setting_music_vol(void);
int  of_setting_sfx_vol(void);
int  of_setting_cdda(void);
int  of_setting_pad(void);
int  of_setting_tick_hz(void);        /* 0 = the PIT rate the game programs */
int  of_setting_screen(void);         /* 0 game, 1 terminal, 2 game + log overlay */

#endif /* NIGHT_OF_SAVE_H */

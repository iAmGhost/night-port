/* of_cdda.h -- the Pocket host's CD-DA engine: per-channel rings + two mono MUSIC voices. */
#ifndef NIGHT_OF_CDDA_H
#define NIGHT_OF_CDDA_H

#include <stdint.h>

/* Allocate the rings. Returns 0 on success, -1 when a ring could not be allocated. */
int  of_cdda_boot(void);

/* Desktop witness directory (the rig dump dir); ignored on the device. */
void of_cdda_set_dir(const char *dir);

/* The sys_cd audio backend's play/stop, i.e. what fn_010b1d asked for. */
void of_cdda_play(uint32_t index, uint32_t lba, uint32_t sectors);
void of_cdda_stop(void);

/* Advance the disc clock and refill the rings. Call once per host pump pass. */
void of_cdda_pump(void);

void     of_cdda_set_volume(int volume);         /* MUSIC group volume, 0..255 */
uint32_t of_cdda_pos(void);                      /* sectors consumed */
uint32_t of_cdda_sectors(void);                  /* sectors the track holds */
uint32_t of_cdda_tracks(void);                   /* tracks started since boot */
int      of_cdda_sounding(void);                 /* 1 when the mixer path is live */

#endif /* NIGHT_OF_CDDA_H */

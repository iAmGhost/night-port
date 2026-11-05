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
uint32_t of_cdda_pos(void);
uint32_t of_cdda_ring_ahead(void);   /* ring frames written ahead of what has been consumed */
int32_t  of_cdda_drift(void);          /* mixer position minus the clock estimate */                      /* sectors consumed */
uint32_t of_cdda_sectors(void);                  /* sectors the track holds */
uint32_t of_cdda_tracks(void);                   /* tracks started since boot */
int      of_cdda_sounding(void);                 /* 1 when the mixer path is live */

/* A gain stage with a knee, shared by the music and the sound effects (docs/361).

 * Below the knee a sample is multiplied outright; above it only the excess is carried, at one quarter of its size.
 * Continuous, monotone, never past full scale, and never BELOW the sample it was given -- the last one is what makes
 * a gain knob mean "louder" instead of "squashed".  With KNEE 20480 / SLOPE 4 the curve cannot reach full scale before 212%,
 * which is why 200% is legal on the Pocket and a plain `x * 2` is not.  The measurements behind those numbers
 * (scripts/analysis/gain_scan.py): 6 of the 14 shipped CD-DA tracks already peak at 32767-32768 (0 dBFS), so the
 * music has no headroom to climb into, and the loudest sample the game's own driver ever put on the board is 30720
 * (93.8% of full scale).
 *
 * It is a compressor, not a volume knob: quiet passages come up, loud passages get squeezed instead of cracking.
 * The PC build leaves gains at 100% by default so the differential rig keeps comparing byte-exact streams
 * (docs/332, docs/164). */
#define OF_GAIN_KNEE  20480
#define OF_GAIN_SLOPE 4

int16_t of_gain16(int16_t x, int gain_pct);
void    of_cdda_set_gain(int pct);
void    of_cdda_set_min_want(int n);     /* night.ini DISC_MIN_WANT: sectors of allowance to accumulate before a read (1..12) */
void    of_cdda_set_read_batch(int n);    /* night.ini DISC_BATCH=: sectors per slot read, 1..12 (docs/375) */        /* 100 = unity, the disc exactly as recorded */

#endif /* NIGHT_OF_CDDA_H */
int32_t  of_cdda_head(void);      /* write cursor lead over the mixer voice, ring frames */

/* Frame-budget counters (task-9a, docs/364): fills attempted, their total and worst duration, frames they
 * delivered, `audio_low` = pump visits with under the 0.1 s guard of audio buffered (the mixer about to run
 * dry), `room_hold` = the disc had sectors and the pump was ready but the ring had no room, `clock_behind` =
 * pump visits where the 75 sectors/s disc clock had passed the host's own disc position, and `ahead_min` =
 * the smallest buffered slack the run held, in ring frames (44 100 a second). */
void of_cdda_stats(uint32_t *fills, uint64_t *fill_us, uint32_t *fill_us_max, uint32_t *fill_frames,
                   uint32_t *audio_low, uint32_t *room_hold, uint32_t *clock_behind, uint32_t *ahead_min,
                   uint32_t *pump_visits, uint32_t *dry);

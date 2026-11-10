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
/* 1 while a track is armed and its voices are running. The card-hold rule (docs/413) asks this first so a
 * stop it did not cause is not logged as one it did. */
int of_cdda_running(void);

/* Advance the disc clock and refill the rings. Call once per host pump pass. */
void of_cdda_pump(void);

void     of_cdda_set_volume(int volume);         /* MUSIC group volume, 0..255 */
uint32_t of_cdda_pos(void);
uint32_t of_cdda_ring_ahead(void);
void of_cdda_async_stats(uint32_t *issues, uint32_t *waits, uint32_t *falls, uint32_t *staged_sectors);   /* ring frames written ahead of what has been consumed */
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
void    of_cdda_set_prefill_first(int n); /* night.ini DISC_PREFILL_FIRST: sectors read before the voices start (12..300) */
void    of_cdda_set_min_want(int n);     /* night.ini DISC_MIN_WANT: sectors of allowance to accumulate before a read (1..12) */
void    of_cdda_set_read_batch(int n);

/* 1 just before the per-frame blocking CD read, 0 just after, so the host knows WHICH blocking read is in
 * flight: the OS idle hook must not present during a resource load, which can land mid-draw (docs/426). */
void    of_cdda_set_disc_busy_cb(void (*cb)(int busy));    /* night.ini DISC_BATCH=: sectors per slot read, 1..12 (docs/375) */        /* 100 = unity, the disc exactly as recorded */

#endif /* NIGHT_OF_CDDA_H */
int32_t  of_cdda_head(void);      /* write cursor lead over the mixer voice, ring frames */

/* Frame-budget counters (task-9a, docs/364): fills attempted, their total and worst duration, frames they
 * delivered, `audio_low` = pump visits with under the 0.1 s guard of audio buffered (the mixer about to run
 * dry), `room_hold` = the disc had sectors and the pump was ready but the ring had no room, `clock_behind` =
 * pump visits where the 75 sectors/s disc clock had passed the host's own disc position, and `ahead_min` =
 * the smallest buffered slack the run held, in ring frames (44 100 a second). */
uint32_t of_cdda_deferred(void);
typedef struct {
    int32_t live[2];        /* ring index each channel's voice reads now; <0 = no voice */
    int32_t dpos;           /* frames the cursor moved since the previous sample */
    uint32_t dt_us;
    int32_t behind;         /* frames written but not yet played, the mixer's view */
    uint32_t vroom;         /* free frames in front of the writer, the mixer's view */
    uint32_t model;         /* the port's model of the same thing */
    int32_t e_live;         /* max |sample| in a window AT the voice cursor: 0 while sounding = the pause */
    int32_t e_wtail;        /* max |sample| just behind the write cursor */
    uint32_t starts, stops; /* our own voice (re)starts */
    uint64_t silent_us;     /* time the track was live but nothing sounded */
    uint32_t laps;          /* voice cursor passed the write cursor: ring-loop replay */
    int32_t ahead;          /* frames from the write frontier FORWARD to the voice: the gap that must not be
                             * zero.  behind ~ RING means this is near zero: the voice is about to read audio
                             * nobody has written, which is the pause, seen from the other side. */
    uint32_t voice_full;    /* fills refused because the voice view had no room */
    int sounding;
} of_cdda_voice_t;
void of_cdda_voice_sample(of_cdda_voice_t *o);   /* docs/408: ask the mixer, not the model */
void of_cdda_set_lookahead(int n);   /* sectors of read-ahead past the disc clock (docs/407) */
uint32_t of_cdda_lookahead(void);   /* pump visits held off: ring deep, the read that would fit was tiny (docs/406) */
void of_cdda_stats(uint32_t *fills, uint64_t *fill_us, uint32_t *fill_us_max, uint32_t *fill_frames,
                   uint32_t *audio_low, uint32_t *room_hold, uint32_t *clock_behind, uint32_t *ahead_min,
                   uint32_t *pump_visits, uint32_t *dry);

/* of_cdda.c -- CD-DA on the Pocket: one mono ring per channel, two looping MUSIC voices, refilled from
 * the per-track data slots.  The pattern is openfpgaOS-Quake's src/quake/engine/cd_of.c, adopted for
 * NIGHT's disc (docs/330 §5, docs/331 §5, §8).
 *
 * WHY A RING AND NOT A QUEUE OF CHUNKS
 *   The mixer plays a pool buffer with a start and an end.  Playing back-to-back chunks means the host
 *   must start the next voice in the exact instant the previous one ends, and this host runs on a pump
 *   (spin units at ~490 Hz = 2 ms), so a chunk boundary is up to 2 ms late -- at 44.1 kHz that is ~88
 *   samples missing between every chunk, which is a click once a second.  A voice that is LOOPED over a
 *   ring never stops: the host only has to keep the written region ahead of the voice's own cursor.
 *
 * WHY TWO MONO VOICES
 *   The mixer's voices are mono streams with a pan; a stereo CD-DA pair is two of them hard-panned
 *   (255,0) and (0,255), tagged OF_MIXER_GROUP_MUSIC so one of_mixer_set_group_volume() call is the
 *   music level (and a fade is one call, which is what the game's own music fades want).
 *
 * WHY THE DISC CLOCK IS KEPT EVEN ON THE DESKTOP
 *   The game compares its tick counter against DAT_00053c48 (the track length in seconds) to decide
 *   when a track is done; a host that consumes a track at the wrong rate changes the game's decisions.
 *   So the 75 sectors/s arithmetic below runs identically with or without a mixer -- on the desktop the
 *   samples leave through a WAV witness instead of through fabric.
 */
/* task-9 (docs/371): BRAM hot paths.  The Pocket core has a 14 336-byte zero-wait-state region
 * (pocket/src/sdk/app.ld:29 `APP_BRAM (rwx) : ORIGIN = 0x00004000, LENGTH = 0x3800`) and this port used none of
 * it: the device ELF's sections were .text 503 376 B in SDRAM with an 8 KiB instruction cache, and no
 * .app_fasttext at all.  OF_FASTTEXT (of_fastram.h) places a function there; the header forces 4-byte alignment
 * because fast RAM is uncached, so 64-byte alignment there is pure padding (Duke3D overflowed the region on
 * padding alone).
 *
 * It is applied to the HOST only, never to src/f: the macro must be absent from a desktop build, where the
 * section has no meaning and f.c is shared byte-for-byte with the SDL host. */
#if defined(NIGHT_POCKET) && !defined(OF_PC) && !defined(NIGHT_BRAM_NONE)
/* The PC shim is built with NIGHT_POCKET *and* OF_PC (sdk.mk adds -DOF_PC to the same flag set), so the guard has
 * to exclude the shim explicitly -- otherwise the section attribute reaches Apple Mach-O and every desktop build
 * dies with "mach-o section specifier requires a segment and section separated by a comma". */
#include "of_fastram.h"
#define HOTTEXT OF_FASTTEXT
#else
#define HOTTEXT
#endif

#include "of_cdda.h"
#include "of_disc.h"
#include "of_log.h"

#include "sys/sys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OF_PC
#include "of.h"
#include "of_cache.h"
#include "of_mixer.h"
#endif

#define CD_SRC_RATE       44100
#define CD_RING_SEC       4
#define CD_RING_FRAMES    (CD_SRC_RATE * CD_RING_SEC)     /* 176 400 frames = 4 s per channel */
#define CD_FRAMES_SECTOR  588                             /* one 2352-byte CD frame in samples */
#define CD_BATCH_MAX          27                              /* sectors per slot read; night.ini DISC_BATCH= picks 1..27.
                                                            * 27 * 2352 = 63 504 B is the largest read this OS moves in one op (of_file_async_max_read()
                                                            * = 65 536 B, docs/403) and the sync fread takes the same: at 12 the card still paid ~50
                                                            * ops a second for the 75 sectors/s it owes, and the OP costs (docs/406). */
static int g_batch = CD_BATCH_MAX;
#define CD_MIN_WANT_MAX   12
static int g_min_want = CD_MIN_WANT_MAX;
static int g_lookahead;   /* seeded in of_cdda_boot to the host value below, then night.ini DISC_LOOKAHEAD= may retarget the device */
uint32_t of_cdda_lookahead(void) { return (uint32_t)g_lookahead; }
/* The ramp's slice.  Engaged only when night.ini says DISC_RAMP=1; the shipping shape primes the whole slice in
 * the requesting visit instead (docs/402). */
#define CD_PRIME_SLICE 24                                 /* sectors of priming read per pump visit (docs/398): the ring is filled by a RAMP, not by one frame */
#include "of_save.h"          /* the night.ini knobs -- outside the BRAM guard below, which is false when BRAM is off */

static uint32_t g_prime_left;

/* ---- async prefetch stages (docs/403) -----------------------------------------------------
 * Two buffers, each one async read wide: 27 sectors = 63 504 B, the largest request this OS accepts
 * (of_file_async_max_read() = 65 536 B).  The core consumes one buffer while the SD DMA fills the other, so a
 * 3.9 ms read stops being a stall the mixer has to survive.  Plain .bss, not of_file_dma_stage_alloc(): the SDK
 * documents that a destination in normal SDRAM is copied from its internal CRAM0 bounce buffer on completion,
 * and one copy per 27 sectors is a cost I can see and measure, whereas a second allocator to get right is not. */
#define CD_STAGE_SECTORS 27                                    /* 27 * 2352 = 63 504 B */
static uint8_t  g_stage_buf[2][CD_STAGE_SECTORS * 2352];
static uint32_t g_stage_n[2], g_stage_i[2];                    /* sectors landed, next to consume */
static int      g_stage_inflight[2], g_stage_cur;
static uint32_t g_stage_max;                                   /* min(CD_STAGE_SECTORS, what the OS allows) */
static int      g_async;                                       /* 0 = the synchronous shape (unchanged code path) */
static uint32_t g_async_issues, g_async_waits, g_async_falls;
static uint32_t g_async_wait_streak;                            /* fills refused because nothing had landed, in a row */                             /* sectors of priming still owed after a track starts */

static int g_prefill_first = 300;                         /* 0.1.74 cut this to 24 and the ring STARVED: audio_low=39, dry=33, ahead_min=0.00 s on hardware (docs/397). 150 (2 s primed) measured zero underruns; its cost is a rare 424 ms fill, and the fix for that is to spread the prefill across frames, not to prime less. */                         /* docs/396: the device measures 3.1 ms per 12-sector batch and 99 ms in its worst read, and a 150-sector prefill is 13 of those batches inside ONE frame -- measured at 423 998 us, which is the audible music pause. 24 sectors = 2 batches (400 ms of audio primed) costs ~6 ms typically and bounds one frame's stall instead of spending 0.42 s in it. */                         /* 2 s read before the voices start; 300/150/90/45 measured, 150 chosen for slack at equal frame cost (docs/376) */                        /* sectors read before the voices start (docs/376); 300 = the old whole-ring prefill */                /* sectors of allowance to accumulate before reading (docs/375 3) */                        /* 1 reproduces the one-sector-at-a-time shape exactly */
#define CD_CHUNK_SECTORS  12                              /* per pump pass: 7056 samples = 160 ms of big sequential reads */
/* The read-ahead past the disc clock.  Chosen BY HOST, deliberately: a runtime setter that runs on both hosts
 * silently moved the desktop shim's audio bytes (docs/407) -- the shim is the byte-exact basis for docs/365/382
 * and must not inherit a device tuning decision.  The shim keeps the 120 (1.6 s) every rig run was made with;
 * the device takes 240 (3.2 s), because 120 was SMALLER than the 4 s (300-sector) ring it feeds: once g_pos
 * touched that ceiling every pump visit could take only the trickle the disc clock releases
 * (75 sectors/s over a 490 Hz tick = 0.15 sectors, truncated to 1), which is the whole explanation of the
 * 1.3-1.7 sectors/op the card measured (docs/406 3).  night.ini DISC_LOOKAHEAD= can still retarget the device. */
#ifdef OF_PC
#define CD_LOOKAHEAD_SECTORS 120
#else
#define CD_LOOKAHEAD_SECTORS 240
#endif
#define CD_MIXER_PRIORITY 0

static uint32_t g_lba, g_sectors, g_pos;                 /* disc coordinates */
static uint64_t g_budget, g_clock_us;                    /* 75 sectors/s from the host clock */
static uint32_t g_tracks;

static int16_t *g_ring[2];                               /* [0]=left, [1]=right, +1 guard frame */
static uint32_t g_write;                                 /* write cursor in frames */
static uint32_t g_done_frames;                           /* frames taken from the disc */
static int      g_running;
static int      g_vol = 255;
static int      g_gain_pct = 100;                /* of_cdda_set_gain(); 100 = unity */

#ifndef OF_PC
static of_mixer_handle_t g_voice[2];
#else
static const char *g_dir;                                /* desktop witness directory */
static FILE *g_wav;
#endif

static int32_t  g_prev_live = -1;
static uint32_t g_prev_us;
static uint32_t g_voice_starts, g_voice_stops, g_voice_laps, g_voice_full;
static uint64_t g_silent_us;

static void voice_start(void);
static void voice_stop(void);
static void cdda_resume(void);

int of_cdda_boot(void) {
    g_lookahead = CD_LOOKAHEAD_SECTORS;   /* the host default (docs/407): 120 on the shim, 240 on the device */
#ifndef OF_PC
    /* The rings must live where the mixer's DMA reads them: its SDRAM pool.  The extra frame past the
     * loop end is the seam guard (the mixer may read the loop end one sample in). */
    g_ring[0] = (int16_t *)of_mixer_alloc_samples((CD_RING_FRAMES + 1) * sizeof(int16_t));
    g_ring[1] = (int16_t *)of_mixer_alloc_samples((CD_RING_FRAMES + 1) * sizeof(int16_t));
#else
    g_ring[0] = (int16_t *)malloc((CD_RING_FRAMES + 1) * sizeof(int16_t));
    g_ring[1] = (int16_t *)malloc((CD_RING_FRAMES + 1) * sizeof(int16_t));
#endif
#ifndef OF_PC
    g_voice[0] = g_voice[1] = OF_MIXER_HANDLE_INVALID;
#endif
    if (!g_ring[0] || !g_ring[1]) {
        printf("cdda: ring allocation failed (%u KB needed)\n",
               (unsigned)(2 * (CD_RING_FRAMES + 1) * sizeof(int16_t) / 1024));
        return -1;
    }
    printf("cdda: two mono rings, %u frames (%.1f s) each = %u KB\n",
           (unsigned)CD_RING_FRAMES, CD_RING_FRAMES / (double)CD_SRC_RATE,
           (unsigned)(2 * (CD_RING_FRAMES + 1) * sizeof(int16_t) / 1024));
    return 0;
}

#ifdef OF_PC
void of_cdda_set_dir(const char *dir) { g_dir = dir; }

/* The witness the audio comparison reads: 44.1 kHz stereo s16, exactly the bytes the mixer would have
 * played.  (scripts/rig/diff_audio.py re-derives the rate from the timeline, but the header is honest.) */
static void wav_open(void) {
    if (!g_dir) { printf("cdda: no witness directory set\n"); return; }
    char path[600];
    snprintf(path, sizeof path, "%s/pcm_cd.wav", g_dir);
    g_wav = fopen(path, "wb");
    if (!g_wav) return;
    unsigned char h[44];
    uint32_t size = 0x7fffffff, fmt = 16, rate = CD_SRC_RATE, byterate = CD_SRC_RATE * 4, data = 0;
    uint16_t pcm = 1, ch = 2, align = 4, bits = 16;
    memcpy(h + 0, "RIFF", 4); memcpy(h + 4, &size, 4); memcpy(h + 8, "WAVEfmt ", 8);
    memcpy(h + 16, &fmt, 4); memcpy(h + 20, &pcm, 2); memcpy(h + 22, &ch, 2);
    memcpy(h + 24, &rate, 4); memcpy(h + 28, &byterate, 4);
    memcpy(h + 32, &align, 2); memcpy(h + 34, &bits, 2);
    memcpy(h + 36, "data", 4); memcpy(h + 40, &data, 4);
    fwrite(h, 1, 44, g_wav);
}
static uint32_t g_wav_bytes;
static void wav_write_patch(void) {
    if (!g_wav) return;
    /* The sizes are only known at the end, so they are written back into the 44-byte header -- the same
     * fix-up src/app/rig_sink.c does for its own witness WAV.  The write cursor must be put back where
     * it was, otherwise every patch makes the following append overwrite the span it just wrote. */
    long tail = ftell(g_wav);
    uint32_t riff = 36 + g_wav_bytes, data = g_wav_bytes;
    fseek(g_wav, 4, SEEK_SET);  fwrite(&riff, 1, 4, g_wav);
    fseek(g_wav, 40, SEEK_SET); fwrite(&data, 1, 4, g_wav);
    fseek(g_wav, tail, SEEK_SET);
    fflush(g_wav);
}
static void wav_close(void) {
    printf("cdda: witness closed after %u bytes of PCM\n", g_wav_bytes);
    wav_write_patch();
    if (g_wav) { fclose(g_wav); g_wav = (FILE *)0; }
}
#endif

static void voice_start(void) {
    g_voice_starts++;       /* OUR voice, not the game's MSCDEX call -- a resume from cdda_resume() lands here too */
#ifdef OF_PC
    g_wav_bytes = 0;
    wav_open();
#else
    g_voice[0] = of_mixer_alloc_for_group_h(OF_MIXER_GROUP_MUSIC, (const uint8_t *)g_ring[0],
                                            CD_RING_FRAMES, CD_SRC_RATE, CD_MIXER_PRIORITY, 0);
    g_voice[1] = of_mixer_alloc_for_group_h(OF_MIXER_GROUP_MUSIC, (const uint8_t *)g_ring[1],
                                            CD_RING_FRAMES, CD_SRC_RATE, CD_MIXER_PRIORITY, 0);
    if (g_voice[0] == OF_MIXER_HANDLE_INVALID || g_voice[1] == OF_MIXER_HANDLE_INVALID) {
        of_log_line("cdda: no two free MUSIC voices; the track is clocked but not sounded\n");
        voice_stop();
        return;
    }
    /* Say the rate again, out loud.  openfpgaOS-Doom's shim (src/doom/shim/i_sdlsound.c:295) records that the
     * hardware mixer "may ignore the sample_rate arg in of_mixer_play and default to output rate (48 kHz),
     * which plays an 11025 Hz DMX sample ~4.4x too fast".  For CD-DA that is 48 000 / 44 100 = 1.088: music
     * 8.8 % sharp, and -- worse for us -- a voice eating the ring 8.8 % faster than the 44 100 frames/s the
     * refill model credits, which underruns it from the inside and is a second source of buzz. */
    of_mixer_set_rate_h(g_voice[0], CD_SRC_RATE);
    of_mixer_set_rate_h(g_voice[1], CD_SRC_RATE);
    of_mixer_set_loop_h(g_voice[0], 0, (int)CD_RING_FRAMES);
    of_mixer_set_loop_h(g_voice[1], 0, (int)CD_RING_FRAMES);
    of_mixer_set_vol_lr_h(g_voice[0], g_vol, 0);     /* the level the settings already settled on */
    of_mixer_set_vol_lr_h(g_voice[1], 0, g_vol);
    /* The group stays at unity.  Setting it to the same number the voice volumes were set to applied the knob TWICE:
     * the mixer multiplies group by voice, so the old MUSIC_VOL=128 default was 0.50 * 0.50 = 0.25 of the recorded
     * level -- 12 dB of attenuation nobody asked for.  The voice carries the menu knob (group volume alone did not
     * move the music on the device, see of_cdda_set_volume); the group is neutral. */
    of_mixer_set_group_volume(OF_MIXER_GROUP_MUSIC, 255);
#endif
}

static void voice_stop(void) {
    g_voice_stops++;
#ifdef OF_PC
    wav_close();
#else
    for (int c = 0; c < 2; c++) {
        if (g_voice[c] != OF_MIXER_HANDLE_INVALID) {
            of_mixer_stop_h(g_voice[c]);
            g_voice[c] = OF_MIXER_HANDLE_INVALID;
        }
    }
#endif
}

/* How much of the ring the voice has already spent.  The voice loops forever, so its consumption is a
 * function of the time since playback started -- the mixer's own position is used only as a cross-check
 * on the device. */
static uint64_t g_play_us;
static int      g_read_failed;          /* the last fill_sectors() ended on a failed slot read */
static uint64_t g_retry_us;             /* 0 while sounding; otherwise when to try the slot again */                /* wall time the voices started */
static uint64_t g_consumed_frames;        /* frames the voice side has taken out of the ring */

/* --- frame-budget instrument (task-9a, docs/364) ---------------------------------------------
 * The streaming side of the budget, in four counts:
 *   fill calls / fill us      -- one fill is read + de-interleave + ring_write + cache flush, i.e. the whole
 *                                cost of putting a chunk of the disc where the mixer DMA can reach it;
 *   audio_low                 -- pump visits with under the 0.1 s fill guard of audio buffered: an audible gap
 *                                waiting to happen (the buzz docs/338 traced was the fill/play race, not a
 *                                missing read);
 *   room_hold                 -- the disc had sectors left and the pump was ready, but the ring had no room;
 *   clock_behind              -- pump visits where the 75 sectors/s disc clock had passed the host's g_pos;
 *   ring-ahead minimum        -- the tightest slack the run ever held, in ring frames.
 *
 * The first version counted "the ring had no room in front of the voice" as if it were an underrun.  It is not:
 * of_cdda_play() pre-fills the WHOLE ring (176 400 frames, 4 s), so for the first seconds of every track the
 * write cursor is a full lap ahead and the pump legitimately refuses to read.  That showed up as 305 126
 * "blocked" visits in a run that never ran out of audio -- a true fact reported under a misleading name.  The
 * two questions are separate, so they get separate counters:
 *   audio_low   -- the mixer is close to running dry: less than the 0.1 s fill guard of audio is buffered.
 *   room_hold   -- the disc still had sectors and the pump was ready, but the ring had no room: the reads are
 *                  faster than the voice eats them, which is the good direction and the one a deeper ring changes.
 *   clock_behind-- the 75 sectors/s disc clock passed g_pos, i.e. the host fell behind the disc's own schedule.
 *   ahead_min   -- the smallest buffered slack the run ever held, in ring frames (44 100 per second). */
static uint32_t g_fill_calls, g_fill_frames, g_fill_us_max, g_audio_low, g_room_hold, g_clock_behind;
static uint32_t g_fill_deferred;   /* visits skipped: ring deep and the read that would fit was tiny */
static uint32_t g_ahead_min = 0xffffffffu;
static uint32_t g_pump_visits;
static uint32_t g_dry;                          /* clock_consume had to clamp: the voice ate more than was written */

#ifdef OF_PC
/* Rig parity (task-11, docs/365): src/app/main.c writes a `stop` marker on every selection change and feeds
 * rig_cdda_sample() the raw 2352-byte sector it pulled off the disc, which is what fills cdda.raw and the
 * running `bytes` series in cdda.jsonl.  The Pocket host wrote neither, so the two hosts' cdda.jsonl files were
 * not comparable and the desktop rig could not diff the two hosts' delivered audio at all.  Declared the way
 * main.c declares them (rig_sink.c has no header). */
void rig_cdda_marker(long t_ms, long index, uint32_t lba, uint32_t sectors);
void rig_cdda_sample(long t_ms, const void *bytes, unsigned n);
static void rig_stop(void) {
    if (g_dir) rig_cdda_marker((long)(sys_now_us() / 1000ull), -1, g_lba, g_sectors);
}
static void rig_sample(const uint8_t *raw) {
    if (g_dir) rig_cdda_sample((long)(sys_now_us() / 1000ull), raw, 2352);
}
#else
#define rig_stop()          ((void)0)
#define rig_sample(raw)     ((void)0)
#endif
static uint64_t g_fill_us;

/* how much the mixer voice ate, every fill */
static void clock_consume(void) {
    uint64_t now = sys_now_us();
    uint64_t want = (now - g_play_us) * (uint64_t)CD_SRC_RATE / 1000000ull;
    if (want > g_done_frames) {
        /* The hard one.  The voices keep playing at hardware speed whatever the host did, so when the clock says
         * more frames have been eaten than were ever written into the ring, the mixer has looped over samples that
         * are stale or silent -- THAT is the underrun, the event the whole CDDA engine exists to avoid (the buzz
         * docs/338 traced was the mirror-image failure: the write cursor lapping the voice).  Counted before the
         * clamp, because the clamp is exactly what hides it. */
        g_dry++;
        want = g_done_frames;                                /* never consume audio that is not there */
    }
    g_consumed_frames = want;
}

/* the ring headroom read the refill decision is built on */
static uint32_t ring_ahead(void) {
    return (g_done_frames >= g_consumed_frames) ? (uint32_t)(g_done_frames - g_consumed_frames) : 0u;
}

/* Room in front of the write cursor before it would overtake the voice that is reading the ring.
 *
 * The two voices loop over the whole ring (of_mixer_set_loop_h(0, CD_RING_FRAMES)), so the mixer reads forward
 * from its own position forever.  Filling is only safe while the write cursor stays behind that position; if it
 * catches up it rewrites samples the DMA is playing, and the audible result is a buzz/crackle rather than a
 * gap.  A "2 s is enough" prefill plus a wall-clock estimate of what was consumed is not a proof of that
 * ordering on hardware -- the estimate and the mixer can drift apart, and the drift grows exactly when the CPU
 * is busy, which is when the buzz appears (intro, stage 1 start).  So the mixer's own position decides. */
#define CD_GUARD_FRAMES 4410u                     /* 0.1 s of slack between fill and playback */
#ifndef CD_SAFE_AHEAD_FRAMES
#define CD_SAFE_AHEAD_FRAMES (CD_SRC_RATE * 2u)   /* 2 s buffered: deeper than this, a small read can wait */
#endif

#ifndef OF_PC
static int32_t mixer_live_pos(void) {
    int p0 = of_mixer_get_position_h(g_voice[0]);
    int p1 = of_mixer_get_position_h(g_voice[1]);
    if (p0 < 0 && p1 < 0) return -1;              /* no voice running */
    if (p0 < 0) p0 = p1;
    if (p1 < 0) p1 = p0;
    return (int32_t)(p0 < p1 ? p0 : p1);          /* the more conservative of the two channels */
}
#endif

static uint32_t room_before_voice(void) {
#ifndef OF_PC
    int32_t live = mixer_live_pos();
    if (live < 0) return CD_RING_FRAMES;
    uint32_t w = g_write % CD_RING_FRAMES;
    uint32_t behind = (w + CD_RING_FRAMES - (uint32_t)live) % CD_RING_FRAMES;   /* cursor ahead of voice */
    if (behind + CD_GUARD_FRAMES >= CD_RING_FRAMES) return 0u;
    return CD_RING_FRAMES - behind - CD_GUARD_FRAMES;
#else
    return ring_ahead() + 4096u;                  /* the desktop rig has no mixer to read */
#endif
}

/* How far the write cursor leads the voice, in ring frames.  Healthy is between CD_GUARD_FRAMES and half the
 * ring; anything near the ring means the voice is about to wrap into data that was never refreshed. */
int32_t of_cdda_head(void) {
#ifndef OF_PC
    int32_t live = mixer_live_pos();
    if (live < 0) return -1;
    uint32_t w = g_write % CD_RING_FRAMES;
    return (int32_t)((w + CD_RING_FRAMES - (uint32_t)live) % CD_RING_FRAMES);
#else
    return (int32_t)ring_ahead();
#endif
}

/* ------------------------------------------------------------------ voice truth (docs/408)
 * Every counter this file had described the PORT'S MODEL of consumption: the disc clock, the frames written, the
 * frames the wall clock says must have been eaten.  On the 0.1.88 card run every one of them read healthy
 * (audio_low=0, room_hold=0, dry=0, ring pinned at 1.9 s) while the player heard the music stop for seconds and
 * start again.  A model that cannot see the mixer cannot diagnose the mixer, so this sampler asks the mixer four
 * questions once a second, in units the ear agrees with:
 *
 *   is the voice running at all?          sounding / silent_ms
 *   how fast is its cursor moving?        dpos over dt -> a rate factor; 1.088 would be the 48 kHz default Doom
 *                                         warns of, and the ring would drain 8.8% faster than the model credits
 *   what is in the samples under it?      e_live -- max |sample| in a window AT the voice cursor.  A voice
 *                                         advancing through zeros is the audible pause, caught red-handed;
 *                                         a voice advancing through loud samples means the silence is elsewhere
 *   has it lapped the write cursor?       laps -- the ring-loop replay, which is a skip rather than a gap
 *
 * Device-only reads; on the desktop shim every field is computed from the model it already had, so the rig's
 * byte-exact basis is untouched.
 */
#define CD_PROBE_WINDOW 8192u

static int32_t ring_energy(const int16_t *buf, uint32_t from, uint32_t n) {
    int32_t peak = 0;
    if (!buf) return 0;
    for (uint32_t k = 0; k < n; k++) {
        int32_t v = buf[(from + k) % CD_RING_FRAMES];
        if (v < 0) v = -v;
        if (v > peak) peak = v;
    }
    return peak;
}

void of_cdda_voice_sample(of_cdda_voice_t *o) {
    memset(o, 0, sizeof(*o));
    o->model = ring_ahead();
    uint64_t now = sys_now_us();
    if (g_prev_us) o->dt_us = (uint32_t)(now - g_prev_us);
    g_prev_us = (uint32_t)now;
#ifndef OF_PC
    o->live[0] = (g_voice[0] == OF_MIXER_HANDLE_INVALID) ? -1 : of_mixer_get_position_h(g_voice[0]);
    o->live[1] = (g_voice[1] == OF_MIXER_HANDLE_INVALID) ? -1 : of_mixer_get_position_h(g_voice[1]);
    o->sounding = (o->live[0] >= 0 || o->live[1] >= 0);
    int32_t live = mixer_live_pos();
    if (o->sounding && g_sectors) {
        /* cursor speed, as a factor of real time: 1.000 means the mixer eats 44 100 frames a second */
        if (g_prev_live >= 0 && live >= 0 && o->dt_us) {
            int32_t d = live - g_prev_live;
            if (d < 0) d += (int32_t)CD_RING_FRAMES;      /* it lapped the ring once between samples */
            if (d > (int32_t)CD_RING_FRAMES / 2) d = (int32_t)CD_RING_FRAMES;   /* nonsense: saturate */
            o->dpos = d;
        }
        o->behind = of_cdda_head();
        o->vroom = room_before_voice();
        /* behind and ahead are the same distance read from the two ends.  behind near RING is not "plenty
         * buffered", it is "the voice is about to run off the written frontier into silence" -- the mistake
         * this port made for several builds was reading that number as a depth (docs/409). */
        o->ahead = (o->behind >= 0) ? (int32_t)(CD_RING_FRAMES - (uint32_t)o->behind) : 0;
        uint32_t w = g_write % CD_RING_FRAMES;
        if (live >= 0) {
            /* Did the voice pass the write cursor since the last look?  The ring looping under the writer is a
             * replay of audio already heard -- audibly a skip, and the one thing the loop design lets happen. */
            if (g_prev_live >= 0 && live > (int32_t)w && g_prev_live <= (int32_t)w) g_voice_laps++;
            o->laps = g_voice_laps;
            o->e_live = ring_energy(g_ring[0], (uint32_t)live, CD_PROBE_WINDOW);
        }
        o->e_wtail = ring_energy(g_ring[0], (w + CD_RING_FRAMES - CD_PROBE_WINDOW) % CD_RING_FRAMES,
                                 CD_PROBE_WINDOW);
    } else if (g_sectors) {
        /* The track is live and nothing is sounding: that IS the pause, timed. */
        if (o->dt_us) g_silent_us += o->dt_us;
        o->silent_us = g_silent_us;
        o->laps = g_voice_laps;
        return;
    }
    o->silent_us = g_silent_us;
    o->laps = g_voice_laps;
    g_prev_live = live;
#else
    o->sounding = g_running;
    o->behind = (int32_t)ring_ahead();
    o->vroom = ring_ahead() + 4096u;
    o->ahead = (int32_t)(CD_RING_FRAMES - (uint32_t)ring_ahead());
    uint32_t w = g_write % CD_RING_FRAMES;
    o->e_live = ring_energy(g_ring[0], w, CD_PROBE_WINDOW);
    o->e_wtail = ring_energy(g_ring[0], (w + CD_RING_FRAMES - CD_PROBE_WINDOW) % CD_RING_FRAMES,
                             CD_PROBE_WINDOW);
    (void)now;
#endif
    o->starts = g_voice_starts;
    o->stops = g_voice_stops;
    o->voice_full = g_voice_full;
}


int16_t of_gain16(int16_t x, int gain_pct) {
    if (gain_pct == 100) return x;
    int      sgn = (x < 0) ? -1 : 1;
    int32_t  a = (x < 0) ? -(int32_t)x : (int32_t)x;        /* magnitude: -32768 stays a number, not UB */
    int32_t  v = a * gain_pct / 100;
    if (v > OF_GAIN_KNEE) {
        /* Compress the excess, but never below what the disc recorded.  The first version of this curve pulled a
         * full-scale sample DOWN (32767 -> 27647 at 150%), i.e. a "louder" knob quietly made the loudest hits
         * softer -- a compressor with the makeup gain in the wrong place.  max() against the input is what makes the
         * setting mean "at least as loud as the recording, never past the ceiling" (docs/361 §3). */
        int32_t c = OF_GAIN_KNEE + (v - OF_GAIN_KNEE) / OF_GAIN_SLOPE;
        v = (c < a) ? a : c;
    }
    if (v > 32767) v = 32767;                               /* unreachable below 212%, kept honest rather than trusted */
    return (int16_t)(sgn * v);
}

void of_cdda_set_gain(int pct) { g_gain_pct = pct; }
void of_cdda_set_prefill_first(int n) {                     /* docs/376: the synchronous slice; 300 reproduces the whole-ring prefill */
    g_prefill_first = (n < CD_MIN_WANT_MAX) ? CD_MIN_WANT_MAX : (n > (int)(CD_RING_FRAMES / CD_FRAMES_SECTOR))
                                             ? (int)(CD_RING_FRAMES / CD_FRAMES_SECTOR) : n;
    printf("cdda: prefill slice = %d sectors read before the voices start\n", g_prefill_first);
}

/* The read-ahead ceiling.  A number this load-bearing must not be compiled in for one host and measured on
 * another: the shim is the byte-exact basis for docs/365/382, so it keeps 120 unless NIGHT_DISC_LOOKAHEAD or the
 * ini asks otherwise (docs/407). */
void of_cdda_set_lookahead(int n) {
#ifdef OF_PC
    (void)n;                       /* the shim is the byte-exact rig basis: it keeps CD_LOOKAHEAD_SECTORS (120)
                                    * no matter what the ini says (docs/407) */
    return;
#endif
    g_lookahead = (n < 1) ? 1 : (n > (int)(CD_RING_FRAMES / CD_FRAMES_SECTOR)) ? (int)(CD_RING_FRAMES / CD_FRAMES_SECTOR)
                                                                               : n;
    printf("cdda: lookahead = %d sectors (%.2f s)\n", g_lookahead, g_lookahead / 75.0);
#ifndef OF_PC
    of_log_line("cdda policy: lookahead=%d sectors\n", g_lookahead);
#endif
}

void of_cdda_set_min_want(int n) {                          /* docs/375 3: 1 = read whatever is due (the old shape), 12 = read a chunk when it is due */
    g_min_want = (n < 1) ? 1 : (n > CD_MIN_WANT_MAX) ? CD_MIN_WANT_MAX : n;
    printf("cdda: fill policy = read at least %d sectors when they are due\n", g_min_want);
#ifndef OF_PC
    /* The setting that governs the read cadence used to reach printf only -- i.e. the terminal, never the card
     * log -- so the boot that measured 50 two-sector reads a second could not show which policy produced them
     * (docs/406 5).  Both dials now speak where the card can be read back. */
    of_log_line("cdda policy: min_want=%d batch=%d safe_ahead=%u frames\n", g_min_want, g_batch,
                (unsigned)CD_SAFE_AHEAD_FRAMES);
#endif
}

void of_cdda_set_read_batch(int n) {                          /* docs/375: 1 = the old one-sector reads, 12 = the shipping batch */
    g_batch = (n < 1) ? 1 : (n > CD_BATCH_MAX) ? CD_BATCH_MAX : n;
    printf("cdda: disc read batch = %d sectors per slot read\n", g_batch);
}

/* the sample copy into the 4 s ring, the port's per-fill hot loop */
static void ring_write(const int16_t *stereo, uint32_t frames) {
    uint32_t done = 0;
    while (done < frames) {
        uint32_t w = g_write % CD_RING_FRAMES;
        uint32_t contig = CD_RING_FRAMES - w;
        uint32_t batch = frames - done;
        if (batch > contig) batch = contig;
        for (uint32_t i = 0; i < batch; i++) {
            if (g_gain_pct == 100) {                  /* the common case stays a copy: 88 200 samples a second */
                g_ring[0][w + i] = stereo[(done + i) * 2 + 0];
                g_ring[1][w + i] = stereo[(done + i) * 2 + 1];
            } else {
                g_ring[0][w + i] = of_gain16(stereo[(done + i) * 2 + 0], g_gain_pct);
                g_ring[1][w + i] = of_gain16(stereo[(done + i) * 2 + 1], g_gain_pct);
            }
        }
#ifndef OF_PC
        of_cache_flush_range(&g_ring[0][w], batch * sizeof(int16_t));
        of_cache_flush_range(&g_ring[1][w], batch * sizeof(int16_t));
        if (w == 0) {                                   /* keep the seam guard in sync with frame 0 */
            g_ring[0][CD_RING_FRAMES] = g_ring[0][0];
            g_ring[1][CD_RING_FRAMES] = g_ring[1][0];
            of_cache_flush_range(&g_ring[0][CD_RING_FRAMES], sizeof(int16_t));
            of_cache_flush_range(&g_ring[1][CD_RING_FRAMES], sizeof(int16_t));
        }
#endif
#ifdef OF_PC
        if (g_wav) {
            for (uint32_t i = 0; i < batch; i++) {
                int16_t pair[2] = { g_ring[0][w + i], g_ring[1][w + i] };
                fwrite(pair, 1, sizeof pair, g_wav);
                g_wav_bytes += (uint32_t)sizeof pair;
            }
        }
#endif
        g_write += batch;
        done += batch;
    }
#ifdef OF_PC
    wav_write_patch();          /* keep the header valid after every span, not only at close */
#endif
}

/* Take `sectors` off the disc and land them in the rings, de-interleaved into the two channels. */
/* Make the ring honest silence.  A looping voice over a ring that was never written -- or over the half of it
 * a failed prefill never reached -- plays whatever the allocator left there, and that is the buzz: it is the
 * same mechanism twice now, once as the zero-length refusal (docs/337) and once as the partial prefill of
 * 0.1.20, which started 111 sectors of real audio inside a 176 400-frame ring and let the voice loop the
 * remaining 1.5 s of nothing.  Silence can only sound like a gap. */
static void ring_silence(void) {
    if (!g_ring[0] || !g_ring[1]) return;
    size_t n = (CD_RING_FRAMES + 1) * sizeof(int16_t);
    memset(g_ring[0], 0, n);
    memset(g_ring[1], 0, n);
#ifndef OF_PC
    of_cache_flush_range(g_ring[0], n);
    of_cache_flush_range(g_ring[1], n);
#endif
}

#ifndef OF_PC
/* Keep a read in flight in every stage that is empty, bounded by the room the voice can still take.  Staged
 * sectors count against that room: they are on their way into the ring and must not be double-promised. */
static void stage_issue(int st) {
    if (g_stage_inflight[st] || g_stage_i[st] < g_stage_n[st]) return;         /* busy, or still has data */
    int other = 1 - st;
    uint32_t staged = (g_stage_i[other] < g_stage_n[other]) ? (g_stage_n[other] - g_stage_i[other]) : 0;
    uint32_t room = room_before_voice() / CD_FRAMES_SECTOR;
    if (room <= staged) return;
    room -= staged;
    uint32_t left = (g_pos + staged < g_sectors) ? (g_sectors - g_pos - staged) : 0;
    if (room > left) room = left;
    if (room > g_stage_max) room = g_stage_max;
    if (!room) return;
    if (of_disc_read_audio_async(g_lba + g_pos + staged, g_stage_buf[st], room) < 0) {
        g_async_falls++;
        /* A fuse, not a hedge.  If the async path never gets a single read away, this code would stop reading the
         * disc at all and the music would simply die -- a worse failure than the stall it was meant to remove.
         * So after eight refusals with nothing issued, the track reverts to the synchronous shape it had before. */
        if (g_async_falls >= 8u && !g_async_issues) {
            g_async = 0;
            for (int i = 0; i < 2; i++) { g_stage_n[i] = 0; g_stage_i[i] = 0; g_stage_inflight[i] = 0; }
            of_log_line("cdda: async prefetch refused 8 times with nothing issued -- reverting to synchronous reads\n");
        }
        return;
    }
    g_stage_inflight[st] = 1; g_stage_n[st] = room; g_stage_i[st] = 0; g_async_issues++;
}

/* The next sector's bytes, from whichever stage has landed data.  A stage still in flight is polled, not waited
 * for: if nothing has landed the caller stops filling, which is the same refusal shape the sync path has. */
static uint8_t *stage_take(void) {
    for (int try2 = 0; try2 < 2; try2++) {
        int st = g_stage_cur;
        if (g_stage_inflight[st] && !of_disc_async_busy()) g_stage_inflight[st] = 0;
        if (!g_stage_inflight[st] && g_stage_i[st] < g_stage_n[st]) return g_stage_buf[st] + g_stage_i[st] * 2352;
        g_stage_cur = 1 - st;
    }
    return 0;
}

static void stage_reset(void) {
    for (int i = 0; i < 2; i++) { g_stage_n[i] = 0; g_stage_i[i] = 0; g_stage_inflight[i] = 0; }
    g_stage_cur = 0;
}
#endif

/* the 2352-byte slot reads that feed the ring: the SD-facing hot path */
static uint32_t fill_sectors_raw(uint32_t sectors) {
    g_read_failed = 0;
    /* Bounded by what the voice still has to read, not just by what the disc has left. */
    if (sectors && room_before_voice() < CD_FRAMES_SECTOR) {
#ifndef OF_PC
        g_voice_full++;     /* the voice view said "no room": counted, because this refusal used to be invisible
                             * and it is the one the writer lives on (docs/408) */
#endif
        return 0;
    }
    static int16_t stereo[CD_FRAMES_SECTOR * 2];
    /* Batched slot reads (docs/375): the old loop asked for one 2352-byte sector at a time, so a 12-sector chunk
     * cost 12 read operations and a 300-sector prefill cost 300 -- each one a seek decision and an OS call.  The
     * batch buffer holds CD_READ_BATCH sectors and is emptied sector by sector, so the byte order, the rig's
     * per-sector witness and the ring's write pattern are untouched; only the number of reads changes. */
    static uint8_t batch[CD_BATCH_MAX * 2352];
    uint32_t frames = 0;
    for (uint32_t s = 0; s < sectors; s++) {
        uint8_t *raw;
#ifndef OF_PC
        /* Prefetch was tried on hardware and the card said no (docs/404): the reads were issued but
         * of_file_async_busy() never fell, so no stage ever held landed bytes, mean_op stayed 0, and the fill
         * path I wrote returned without reading -- music did not play at all.  The branch is kept so the shape
         * can be re-tried on a newer OS, but it must always fall through to the synchronous read below, and it
         * is off by default (night.ini DISC_ASYNC). */
        raw = 0;
        if (g_async) {
            stage_issue(g_stage_cur);
            stage_issue(1 - g_stage_cur);
            raw = stage_take();
            if (raw) { g_stage_i[g_stage_cur]++; g_async_wait_streak = 0; }
            else {
                g_async_waits++;
                if (++g_async_wait_streak >= 8u) g_async = 0;   /* losing: stop pretending, just read */
            }
        }
        if (!raw)
#else
        if (1)
#endif
        {
            raw = batch;
            if ((s % (uint32_t)g_batch) == 0) {
                uint32_t n = sectors - s;
                if (n > (uint32_t)g_batch) n = (uint32_t)g_batch;
                if (of_disc_read_audio_run(g_lba + g_pos, batch, n) != 0) { g_read_failed = 1; return frames; }
            } else {
                raw = batch + (s % (uint32_t)g_batch) * 2352;
            }
        }
        rig_sample(raw);                      /* the same sector bytes the SDL host hands the rig */
        const int16_t *src = (const int16_t *)raw;
        for (int i = 0; i < CD_FRAMES_SECTOR; i++) {
            stereo[i * 2 + 0] = src[i * 2 + 0];
            stereo[i * 2 + 1] = src[i * 2 + 1];
        }
        ring_write(stereo, CD_FRAMES_SECTOR);
        g_pos++;
        g_done_frames += CD_FRAMES_SECTOR;
        frames += CD_FRAMES_SECTOR;
    }
    return frames;
}

/* The timed wrapper.  The blocked case is counted before the clock is read: it is the shape of "the disc had
 * more to give and the ring could not take it", which is what a deeper ring or an async read would fix. */
/* the fill decision wrapper, once per tick (docs/364 3) */
static uint32_t fill_sectors(uint32_t sectors) {
    if (sectors && room_before_voice() < CD_FRAMES_SECTOR) { g_room_hold++; return 0; }
    uint64_t t0 = sys_now_us();
    uint32_t frames = fill_sectors_raw(sectors);
    uint32_t dt = (uint32_t)(sys_now_us() - t0);
    g_fill_calls++; g_fill_frames += frames; g_fill_us += dt;
    if (dt > g_fill_us_max) g_fill_us_max = dt;
    return frames;
}

void of_cdda_play(uint32_t index, uint32_t lba, uint32_t sectors) {
    /* A play request REPLACES whatever is playing -- MSCDEX has one pickup at a time (src/app/main.c:326,
     * docs/105, which is the same fault found in the SDL host: "every selection overlaps the old track").
     * The Pocket host had it too, in its worst form: the rings are one shared pair of buffers, so a
     * replacement allocated a second pair of looping MUSIC voices over the ring the first pair was still
     * reading.  Two copies of the same audio at once, one of them stopped later by pool pressure -- which is
     * exactly "the music plays double simultaneously and one stops later" -- and when the pool has no voice
     * left for the new pair, the new track is clocked but silent, which is part of "music missing". */
    if (g_running) { rig_stop(); voice_stop(); g_running = 0; }
    g_tracks++;
    g_lba = lba; g_sectors = sectors; g_pos = 0; g_budget = 0;
    g_clock_us = sys_now_us();
    g_write = 0; g_done_frames = 0;

#ifdef OF_PC
    void rig_cdda_marker(long t_ms, long idx, uint32_t lba_, uint32_t sec_);
    if (g_dir) rig_cdda_marker((long)(sys_now_us() / 1000ull), (long)index, lba, sectors);
#endif

    of_log_line("cdda req idx=%u lba=%u sectors=%u (%.1f s) -> disc track %d slot %d\n",
         index, lba, sectors, sectors / 75.0, of_disc_track_of_lba(lba),
         of_disc_track_slot((uint32_t)(of_disc_track_of_lba(lba) > 0 ? of_disc_track_of_lba(lba) : 0)));

    /* A zero-length request must not start anything.  fn_010b1d computes the length as
     * rec[idx+1].lba - rec[idx].lba out of the record array fn_010927 wrote into the game's heap, so a
     * zero means the two records are the same LBA -- and a looping voice started over a ring that was
     * never written plays whatever the allocator left there.  That is not silence, it is the buzz: the
     * card log showed request after request with sectors=0 while the two MUSIC voices kept looping, at
     * the logo, through the .fli intro and in the stages, with real music only where the length was
     * non-zero (the title and the PCX slides). */
    if (!sectors) {
        of_log_line("cdda refused: zero-length track (stopping the voices rather than looping an empty ring)\n");
        if (g_running) { voice_stop(); g_running = 0; }
        g_sectors = 0; g_pos = 0; g_done_frames = 0; g_write = 0;
        return;
    }

    if (!g_ring[0] || !g_ring[1]) return;

#ifdef OF_PC
    /* Desktop: open the witness before the first sample is taken so the WAV covers the track from its
     * very first sector.  On the device the voices must not start over an empty ring, so there the
     * pre-fill comes first (see below). */
    voice_start();
#endif

    /* Pre-fill the whole ring before the voices start: a looping voice needs something to loop over, and
     * prefilling is also what proves the slot is readable before any sound is expected. */
    /* Read a SLICE before the voices start, not the whole ring (docs/376).  The full 300-sector prefill was the
     * most expensive thing the audio path did -- 300 sectors (705 KB) read inside the single frame that requested a
     * track, measured as a 5.7 ms fill on the desktop and unknown on the card, where a sector read goes over SPI.
     * A slice is enough for the two things the prefill is actually for: the voice needs something real to loop over
     * the moment it starts, and a readable slot must be proven before sound is expected.  The pump then tops the
     * ring up in chunks, and it does that comfortably: it keeps g_pos at budget + 120 sectors, while the voice sits
     * at the budget, so a slice of 45 sectors (0.6 s) is overtaken by the pump within a few ticks.  Everything below
     * this point keeps its meaning: zero sectors read is still a refusal, and a short read is still logged as
     * partial with the rest of the ring silenced rather than left as leftovers. */
    uint32_t prefill_target = CD_RING_FRAMES / CD_FRAMES_SECTOR;          /* 300 sectors = the whole ring */
    if (prefill_target > sectors) prefill_target = sectors;
    uint32_t prefill_sectors = (uint32_t)g_prefill_first;
    if (prefill_sectors > prefill_target) prefill_sectors = prefill_target;
    /* What is paid HERE is bounded by CD_PRIME_SLICE; the rest is owed to the pump (g_prime_left).  0.1.75 read
     * all 150 primed sectors inside the single frame that requested the track -- on the card that is 150 sector
     * reads at ~2.1 ms each, and the log recorded fills up to 418 281 us.  Starting the voices after two batches
     * (~6 ms typically) instead of after a third of a second is the start delay the player hears as the music
     * lagging behind the picture (docs/398). */
    /* REVERTED (docs/402): the card says the ramp is what made the music skip.  With the ramp engaged the fresh
     * log region reads ahead_min=0.16 s at the first track switch and 0.00 s from then on, dry 5->49 and
     * audio_low 5->64 climbing monotonically -- the mixer's looping voice ran out of written ring and wrapped
     * over its own beginning, which is exactly "it plays back or skips the early portion".  Without the ramp
     * (0.1.75) the same counters measured dry=0, audio_low=0, ahead_min 19 715 frames (0.45 s).  Priming the
     * slice in one visit costs a big fill (max 418 281 us, still a real problem) but it leaves the ring AHEAD,
     * and being ahead is what the mixer needs.  DISC_RAMP=1 re-arms the ramp for measurement. */
    if (of_setting_disc_ramp() && prefill_sectors > CD_PRIME_SLICE) prefill_sectors = CD_PRIME_SLICE;
    g_prime_left = of_setting_disc_ramp() ? (prefill_target - prefill_sectors) : 0;
#ifndef OF_PC
    /* The priming stays synchronous: it is the one thing that can put the ring ahead of real time, and 0.1.75's
     * one-shot priming is the shape that measured dry=0, audio_low=0 (docs/402).  Async starts after it, for the
     * steady-state feed -- which is where the 3.9 ms-per-operation reads were stalling the pump 75 times a second. */
    stage_reset();
    g_async = 0;
    if (of_setting_disc_async()) {
        uint32_t m = of_disc_async_max_sectors();
        if (m >= 2) { g_stage_max = (m < CD_STAGE_SECTORS) ? m : CD_STAGE_SECTORS; g_async = 1; }
    }
#endif
    ring_silence();                       /* what the prefill does not reach must be silence, not leftovers */
    uint32_t got_frames = fill_sectors(prefill_sectors);
    if (got_frames == 0) {
        of_log_line("cdda refused: prefill read nothing (0 of %u sectors) -- slot unreadable, voices stay off\n",
                    prefill_sectors);
        g_sectors = 0; g_pos = 0; g_done_frames = 0; g_write = 0;
        return;
    }
    if (got_frames != prefill_sectors * CD_FRAMES_SECTOR) {
        /* Refusing the whole track because the read stopped at sector 111 of 300 was the wrong call: 111
         * sectors is 1.5 s of audio already in the ring, and the pump can top it up from here.  A partial
         * prefill costs one audible hiccup at the very worst; a refusal costs the whole track. */
        of_log_line("cdda: prefill reached %u of %u sectors (slice %u, ring target %u) -- pump will top up\n",
                    got_frames / CD_FRAMES_SECTOR, prefill_sectors, prefill_sectors, prefill_target);
    }

    /* g_write stays where the prefill left it -- it is frames WRITTEN, and must equal g_done_frames.
     *
     * This line used to read `g_write = 0;` with the comment "the voice starts at the first prefilled
     * frame". The voice does start at ring index 0, but zeroing g_write did not move the voice: it moved
     * the WRITER back to index 0, where the voice was already reading, and left frames [88 200, 176 400)
     * as the zeros ring_silence() put there. So the writer chased the voice from behind at the real-time
     * rate (75 sectors/s) instead of leading it by the 2 s the prefill had just bought, the voice walked
     * off the end of the written region into those zeros, and the music stopped for as long as a ring lap
     * takes -- "plays 1-2 seconds then pause a while and continues", at every track start (docs/409).
     * g_done_frames was left at 88 200, so the model believed in a ring the hardware had not filled: the
     * 87 100-frame disagreement between of_cdda_head() and ring_ahead() that docs/408 2 measured and could
     * not name is this line exactly. */
    g_consumed_frames = 0;
    g_play_us = sys_now_us();
#ifndef OF_PC
    voice_start();
#endif
    g_running = 1;
}

void of_cdda_stop(void) {
    if (g_running) { rig_stop(); voice_stop(); g_running = 0; }
    g_sectors = 0; g_pos = 0; g_done_frames = 0; g_prime_left = 0;
#ifndef OF_PC
    stage_reset(); g_async = 0;
#endif
}

void of_cdda_pump(void) {
    if (!g_sectors) return;
    g_pump_visits++;                      /* the denominator for the visit-counters below: of_cdda_pump runs
                                           * inside sys_port_spin, i.e. hundreds of times a frame */
    if (!g_running && g_retry_us && sys_now_us() >= g_retry_us) { cdda_resume(); return; }

    /* The disc's own clock: 75 sectors a second, measured on the host clock.  sys_cd_da_plan caps how far
     * ahead the host may run (src/sys/sys.c) -- the same cushion the SDL host uses. */
    uint64_t now = sys_now_us();
    uint64_t el = (now - g_clock_us) * 75ull / 1000000ull;
    g_clock_us += el * 1000000ull / 75ull;
    g_budget += el;

    clock_consume();

    if (g_running) {                                  /* the budget witnesses, once per pump visit */
        uint32_t a = ring_ahead();
        if (a < g_ahead_min) g_ahead_min = a;
        /* Not at the tail: every track the game re-requests ends with the ring draining before fn_010b1d runs
         * again, and the original's single MSCDEX pickup does the same.  Counting that as a risk buried the ones
         * that matter. */
        if (a < CD_GUARD_FRAMES && g_pos + 2u < g_sectors) g_audio_low++;   /* under 100 ms buffered, data left */
        if ((uint64_t)g_budget > g_pos) g_clock_behind++;   /* the disc clock is ahead of what we have read */
    }

    /* Defer a TINY read while the voice is comfortably behind us (docs/406).  This is the whole cost story on
     * the card: the ring runs to 96.5% full (head=170 086 of 176 400 frames = 3.86 s buffered), so `room` is
     * about 3 sectors, and the pump dutifully reads 2 sectors about 50 times a second -- at 5-26 ms per
     * operation that is 60-65% of wall time spent inside fread, for exactly the 75 sectors/s the disc owes us.
     * The operation costs; the sector does not.  So when the voice is more than CD_SAFE_AHEAD_FRAMES behind,
     * hold off: the ring is already deep enough to ride out several seconds of disc clock, and when it does
     * drain the same bytes arrive in a handful of 27-sector operations instead of a hundred 2-sector ones.
     * The deferral is conditional on being DEEP, so it cannot manufacture an underrun -- when we are low, every
     * read goes through exactly as before. */
#ifndef OF_PC                                       /* device-only: the shim has no mixer position to trust,
                                                     * and it is the byte-exact basis for docs/365/382 */
    {
        int32_t vh32 = of_cdda_head();              /* frames the voice has NOT yet consumed */
        /* 0.1.87 gated this on the MIXER's position alone and the card answered with audio_low 0 -> 7 553,
         * ahead_min 0.35 s -> 0.00 s, dry 353: the voice cursor kept reporting "deep" while the host's own
         * accounting of the ring ran down, so the pump held off on a ring the pump itself thought empty
         * (docs/407).  A deferral is allowed only when BOTH measures agree we are deep -- the host model is
         * the same one that raises audio_low and gates `room`, so they can no longer disagree about this. */
        if (vh32 >= (int32_t)CD_SAFE_AHEAD_FRAMES && ring_ahead() >= CD_SAFE_AHEAD_FRAMES) {
            g_fill_deferred++;
            return;
        }
    }
#endif

    /* Keep the ring between empty and full: never write more than the ring can hold ahead of the voice,
     * and never read faster than the disc runs.  The 2 s prefill is the slack that absorbs an SD read. */
    uint32_t room = (ring_ahead() < CD_RING_FRAMES) ? (CD_RING_FRAMES - ring_ahead()) : 0;
#ifndef OF_PC
    /* Two notions of "consumed" were in play: the host's clock model, and the mixer voice's own position.  The
     * pump used the clock to decide the ring was full, while the voice -- which plays at hardware speed whether
     * the core is busy or not -- had already eaten more than the model credited.  Take the smaller room. */
    uint32_t vroom = room_before_voice();
    if (vroom < room) room = vroom;
#endif
    if (room < CD_FRAMES_SECTOR) { if (g_pos < g_sectors) g_room_hold++; return; }

    /* Priming ramp (docs/398).  Only the prefill can put the ring AHEAD of real time: the steady-state path is
     * gated by the disc clock (75 sectors/s of allowance) and can never overtake it -- which is exactly why 0.1.74,
     * which primed 24 sectors and expected the pump to catch up, underran (audio_low=39, dry=33, ahead_min=0.00 s,
     * docs/397).  So keep the priming deep, the whole 4 s ring the playable build had, and spread it over visits:
     * 24 sectors a visit primes 300 in 13 visits (~27 ms of pump work) instead of one frame paying ~400 ms. */
    if (g_prime_left) {
        uint32_t slice = (g_prime_left < CD_PRIME_SLICE) ? g_prime_left : CD_PRIME_SLICE;
        if (ring_ahead() + (uint64_t)slice * CD_FRAMES_SECTOR < CD_RING_FRAMES) {
            uint32_t primed = fill_sectors(slice);
            if (primed) {
                g_prime_left -= primed / CD_FRAMES_SECTOR;
                if (g_read_failed) g_prime_left = 0;
                return;
            }
        }
        g_prime_left = 0;                       /* nothing left to take: fall through to the budget-limited path */
    }

    uint64_t ceiling = g_budget + (uint64_t)g_lookahead;
    if ((uint64_t)g_pos >= ceiling) return;                  /* already prefetching far enough ahead */
    /* Wait for a whole chunk to be due (docs/375 §6).  The disc clock advances 75 sectors/s while the pump runs at
     * the 490 Hz tick, so `ceiling - g_pos` grows by about 0.15 sectors a visit: truncated to an integer it is
     * almost always 0 or 1, and every steady-state read asked for ONE sector at the disc's own rate -- 7 647 reads
     * for 8 747 sectors over a nine-episode sweep.  The allowance is the same; only its granularity changes: hold the
     * read until CD_MIN_WANT sectors of it have accumulated, then take them in one read.  Two cases must not wait:
     * the track is running out (take what is left), and the ring is close to the guard (take what is due now,
     * because an underrun is audible and a slightly early read is not). */
    uint32_t avail = (uint32_t)(ceiling - g_pos);
    uint32_t want = (avail < (uint32_t)g_min_want) ? avail : (uint32_t)g_min_want;
    if (want < (uint32_t)g_min_want && ring_ahead() > 2u * CD_GUARD_FRAMES &&
        (uint64_t)g_pos + (uint64_t)g_min_want <= (uint64_t)g_sectors)
        return;                                             /* the chunk is not due yet, and there is no reason to hurry */
    uint32_t allowed = sys_cd_da_plan(g_pos, g_sectors, want, CD_CHUNK_SECTORS);
    if (!allowed) return;
    if (allowed * CD_FRAMES_SECTOR > room) allowed = room / CD_FRAMES_SECTOR;
    if (!allowed) return;

    uint32_t got = fill_sectors(allowed);
    if (g_read_failed) {
        /* The slot read broke.  Keep playing anyway and the voice wraps into samples that were never
         * refreshed -- audibly a buzz, and it does not stop until the game replaces the track.  Stop, silence,
         * and come back when the slot answers again: a gap is a smaller lie than a buzz. */
        if (g_running) {
            voice_stop(); g_running = 0;
            ring_silence();
            g_write = 0; g_done_frames = 0; g_consumed_frames = 0;
            of_log_line("cdda: slot read failed at sector %u of %u -- voices stopped; retrying in 1 s\n",
                        g_pos, g_sectors);
        }
        g_retry_us = sys_now_us() + 1000000ull;
        return;
    }
    if (got == 0 && g_pos >= g_sectors) { of_cdda_stop(); return; }  /* the track ran out; the game restarts it */
}

/* Coming back after a failed read: prefill half the ring from where the disc position stopped, then start the
 * voices again.  Late by seconds, but continuous and clean. */
static void cdda_resume(void) {
    uint32_t resume_target = (CD_RING_FRAMES / 2) / CD_FRAMES_SECTOR;
    if (resume_target > g_sectors - g_pos) resume_target = g_sectors - g_pos;
    if (!resume_target) { of_cdda_stop(); return; }
    uint32_t want = (of_setting_disc_ramp() && resume_target > CD_PRIME_SLICE) ? CD_PRIME_SLICE : resume_target;
    ring_silence();
    g_write = 0; g_done_frames = 0; g_consumed_frames = 0;
    uint32_t got = fill_sectors(want);
    if (got == 0) { g_retry_us = sys_now_us() + 2000000ull; return; }   /* still broken: back off */
    g_prime_left = (resume_target > got / CD_FRAMES_SECTOR) ? resume_target - got / CD_FRAMES_SECTOR : 0;
    g_play_us = sys_now_us();
    voice_start();
    g_running = 1;
    g_retry_us = 0;
    of_log_line("cdda: slot answering again, resuming at sector %u of %u\n", g_pos, g_sectors);
}

void     of_cdda_set_volume(int v) {
    g_vol = v & 0xff;
#ifndef OF_PC
    /* A menu knob has to be audible the moment it moves.  Group volume alone was not enough on the device --
     * the music kept its level when the slider moved -- so the level also goes onto each voice, which is the
     * API openfpgaOS-Doom drives (i_sdlsound.c:197 of_mixer_set_vol_lr_h). */
    of_mixer_set_group_volume(OF_MIXER_GROUP_MUSIC, 255);   /* neutral: the voice carries the knob, once */
    if (g_voice[0] != OF_MIXER_HANDLE_INVALID) of_mixer_set_vol_lr_h(g_voice[0], g_vol, 0);
    if (g_voice[1] != OF_MIXER_HANDLE_INVALID) of_mixer_set_vol_lr_h(g_voice[1], 0, g_vol);
#endif
}
uint32_t of_cdda_ring_ahead(void) { return ring_ahead(); }

void of_cdda_async_stats(uint32_t *issues, uint32_t *waits, uint32_t *falls, uint32_t *staged_sectors) {
    if (issues) *issues = g_async_issues;
    if (waits)  *waits  = g_async_waits;
    if (falls)  *falls  = g_async_falls;
    uint32_t st = 0;
#ifndef OF_PC
    for (int i = 0; i < 2; i++) if (g_stage_i[i] < g_stage_n[i]) st += g_stage_n[i] - g_stage_i[i];
#endif
    if (staged_sectors) *staged_sectors = st;
}

uint32_t of_cdda_deferred(void) { return g_fill_deferred; }

void of_cdda_stats(uint32_t *fills, uint64_t *fill_us, uint32_t *fill_us_max, uint32_t *fill_frames,
                   uint32_t *audio_low, uint32_t *room_hold, uint32_t *clock_behind, uint32_t *ahead_min,
                   uint32_t *pump_visits, uint32_t *dry) {
    if (fills) *fills = g_fill_calls;
    if (fill_us) *fill_us = g_fill_us;
    if (fill_us_max) *fill_us_max = g_fill_us_max;
    if (fill_frames) *fill_frames = g_fill_frames;
    if (audio_low) *audio_low = g_audio_low;
    if (room_hold) *room_hold = g_room_hold;
    if (clock_behind) *clock_behind = g_clock_behind;
    if (ahead_min) *ahead_min = (g_ahead_min == 0xffffffffu) ? 0u : g_ahead_min;
    if (pump_visits) *pump_visits = g_pump_visits;
    if (dry) *dry = g_dry;
}
int32_t  of_cdda_head(void);
uint32_t of_cdda_pos(void)     { return g_pos; }
uint32_t of_cdda_sectors(void) { return g_sectors; }
uint32_t of_cdda_tracks(void)  { return g_tracks; }
int      of_cdda_sounding(void){ return g_running; }

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
#define CD_CHUNK_SECTORS  12                              /* per pump pass: 7056 samples = 160 ms of big sequential reads */
#define CD_LOOKAHEAD_SECTORS 120                          /* 1.6 s of audio read past the disc clock: the slack that
                                                           * survives a blocking video present without underrunning the
                                                           * voice.  Reading only as far as the clock has run leaves the
                                                           * ring tens of milliseconds deep, and the log shows it: the
                                                           * first card run reported ring=2352 frames (53 ms) of slack
                                                           * while one present cost 88 ms -- that underrun is the buzz. */
#define CD_MIXER_PRIORITY 0

static uint32_t g_lba, g_sectors, g_pos;                 /* disc coordinates */
static uint64_t g_budget, g_clock_us;                    /* 75 sectors/s from the host clock */
static uint32_t g_tracks;

static int16_t *g_ring[2];                               /* [0]=left, [1]=right, +1 guard frame */
static uint32_t g_write;                                 /* write cursor in frames */
static uint32_t g_done_frames;                           /* frames taken from the disc */
static int      g_running;
static int      g_vol = 255;

#ifndef OF_PC
static of_mixer_handle_t g_voice[2];
#else
static const char *g_dir;                                /* desktop witness directory */
static FILE *g_wav;
#endif

static void voice_start(void);
static void voice_stop(void);
static void cdda_resume(void);

int of_cdda_boot(void) {
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
    of_mixer_set_group_volume(OF_MIXER_GROUP_MUSIC, g_vol);
#endif
}

static void voice_stop(void) {
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

static void clock_consume(void) {
    uint64_t now = sys_now_us();
    uint64_t want = (now - g_play_us) * (uint64_t)CD_SRC_RATE / 1000000ull;
    if (want > g_done_frames) want = g_done_frames;         /* never consume audio that is not there */
    g_consumed_frames = want;
}

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

static void ring_write(const int16_t *stereo, uint32_t frames) {
    uint32_t done = 0;
    while (done < frames) {
        uint32_t w = g_write % CD_RING_FRAMES;
        uint32_t contig = CD_RING_FRAMES - w;
        uint32_t batch = frames - done;
        if (batch > contig) batch = contig;
        for (uint32_t i = 0; i < batch; i++) {
            g_ring[0][w + i] = stereo[(done + i) * 2 + 0];
            g_ring[1][w + i] = stereo[(done + i) * 2 + 1];
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

static uint32_t fill_sectors(uint32_t sectors) {
    g_read_failed = 0;
    /* Bounded by what the voice still has to read, not just by what the disc has left. */
    if (sectors && room_before_voice() < CD_FRAMES_SECTOR) return 0;
    static int16_t stereo[CD_FRAMES_SECTOR * 2];
    static uint8_t raw[2352];
    uint32_t frames = 0;
    for (uint32_t s = 0; s < sectors; s++) {
        if (of_disc_read_audio(g_lba + g_pos, raw) != 0) { g_read_failed = 1; return frames; }
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

void of_cdda_play(uint32_t index, uint32_t lba, uint32_t sectors) {
    /* A play request REPLACES whatever is playing -- MSCDEX has one pickup at a time (src/app/main.c:326,
     * docs/105, which is the same fault found in the SDL host: "every selection overlaps the old track").
     * The Pocket host had it too, in its worst form: the rings are one shared pair of buffers, so a
     * replacement allocated a second pair of looping MUSIC voices over the ring the first pair was still
     * reading.  Two copies of the same audio at once, one of them stopped later by pool pressure -- which is
     * exactly "the music plays double simultaneously and one stops later" -- and when the pool has no voice
     * left for the new pair, the new track is clocked but silent, which is part of "music missing". */
    if (g_running) { voice_stop(); g_running = 0; }
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
    uint32_t prefill_sectors = CD_RING_FRAMES / CD_FRAMES_SECTOR;         /* 300 sectors = 4 s */
    if (prefill_sectors > sectors) prefill_sectors = sectors;
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
        of_log_line("cdda: partial prefill (%u of %u sectors) -- starting with what arrived, pump will top up\n",
                    got_frames / CD_FRAMES_SECTOR, prefill_sectors);
    }

    g_write = 0;                     /* the voice starts at the first prefilled frame */
    g_consumed_frames = 0;
    g_play_us = sys_now_us();
#ifndef OF_PC
    voice_start();
#endif
    g_running = 1;
}

void of_cdda_stop(void) {
    if (g_running) { voice_stop(); g_running = 0; }
    g_sectors = 0; g_pos = 0; g_done_frames = 0;
}

void of_cdda_pump(void) {
    if (!g_sectors) return;
    if (!g_running && g_retry_us && sys_now_us() >= g_retry_us) { cdda_resume(); return; }

    /* The disc's own clock: 75 sectors a second, measured on the host clock.  sys_cd_da_plan caps how far
     * ahead the host may run (src/sys/sys.c) -- the same cushion the SDL host uses. */
    uint64_t now = sys_now_us();
    uint64_t el = (now - g_clock_us) * 75ull / 1000000ull;
    g_clock_us += el * 1000000ull / 75ull;
    g_budget += el;

    clock_consume();

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
    if (room < CD_FRAMES_SECTOR) return;

    uint64_t ceiling = g_budget + CD_LOOKAHEAD_SECTORS;
    if ((uint64_t)g_pos >= ceiling) return;                  /* already prefetching far enough ahead */
    uint32_t allowed = sys_cd_da_plan(g_pos, g_sectors, (uint32_t)(ceiling - g_pos), CD_CHUNK_SECTORS);
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
    uint32_t want = (CD_RING_FRAMES / 2) / CD_FRAMES_SECTOR;
    if (want > g_sectors - g_pos) want = g_sectors - g_pos;
    if (!want) { of_cdda_stop(); return; }
    ring_silence();
    g_write = 0; g_done_frames = 0; g_consumed_frames = 0;
    if (fill_sectors(want) == 0) { g_retry_us = sys_now_us() + 2000000ull; return; }   /* still broken: back off */
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
    of_mixer_set_group_volume(OF_MIXER_GROUP_MUSIC, g_vol);
    if (g_voice[0] != OF_MIXER_HANDLE_INVALID) of_mixer_set_vol_lr_h(g_voice[0], g_vol, 0);
    if (g_voice[1] != OF_MIXER_HANDLE_INVALID) of_mixer_set_vol_lr_h(g_voice[1], 0, g_vol);
#endif
}
uint32_t of_cdda_ring_ahead(void) { return ring_ahead(); }
int32_t  of_cdda_head(void);
uint32_t of_cdda_pos(void)     { return g_pos; }
uint32_t of_cdda_sectors(void) { return g_sectors; }
uint32_t of_cdda_tracks(void)  { return g_tracks; }
int      of_cdda_sounding(void){ return g_running; }

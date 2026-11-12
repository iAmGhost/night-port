#ifndef CD_ISO_H
#define CD_ISO_H
#include <stdint.h>

/* Modern CD backend for the port: reads NIGHT's real disc (TG_NIGHT.CUE + TG_NIGHT.IMG) and answers
 * the MSCDEX-style requests fn_010927 (TOC header) and fn_107b9 (TOC entry) would have made.
 * See cd_iso.c for the request/reply decoding and why the disc-present gate fn_010c3e passes on the
 * real pressing. Returns 0 on success, -1 if the cue/img cannot be opened. */
int cd_iso_open(const char *cue_path);
void cd_iso_close(void);

/* Where the bytes actually come from: the image named by the CUE, or the directory of per-track
 * trackNN.bin files NIGHT_CD_DIR selected (or that was found beside the CUE when there was no image),
 * or "none" when the disc is not there at all.  Nothing in the game sees it; it exists so a boot can
 * say which backend answered. */
const char *cd_iso_source(void);

/* The disc may be absent and the game still has to start: cd_iso_open() falls back to the geometry in
 * src/gen/disc_toc.h (scripts/gen/gen_disc_toc.py, docs/455) so the disc-present gate fn_010c3e passes
 * and the CD-DA reads are refused.  Two switches, and they ask different questions:
 *
 *   NIGHT_NO_DISC=1      treat the drive as EMPTY -- the cue, image and track files are not opened even
 *                        when they are present, which is how the no-disc path is measured with the tree
 *                        still on disk.
 *   NIGHT_REQUIRE_DISC=1 nothing is substituted for the missing disc, so fn_010c3e shows cdinsert.pcx
 *                        -- the original's answer to an empty drive.
 *
 * NO_DISC alone therefore runs silent; NO_DISC + REQUIRE gives the authentic insert-CD screen. */
int cd_iso_no_disc(void);         /* 1 = the TOC is the compiled-in table, nothing was parsed */
int cd_iso_require_disc(void);    /* NIGHT_REQUIRE_DISC was set -- outranks NIGHT_NO_DISC */
int cd_iso_no_disc_wanted(void);  /* NIGHT_NO_DISC was set: the drive is treated as empty */
int cd_iso_disc_absent(void);     /* 1 = nothing here can answer a read, substituted TOC or not */
int cd_iso_has_audio(void);       /* 0 = nothing can answer a read: a boot with no music */
int cd_iso_tracks_missing(void);  /* audio tracks with no trackNN.bin in the per-track tree */
int cd_iso_tracks_total(void);

uint32_t cd_iso_track_count(void);
uint32_t cd_iso_first_track(void);
uint32_t cd_iso_last_track(void);
uint32_t cd_iso_total_sectors(void);
int32_t  cd_iso_track_lba(uint32_t track);           /* disc LBA of that track's INDEX 01 */

/* sys_cd backend callbacks (sys_cd_set_backend): the TOC header (7 bytes) and the 5-byte TOC entry
 * records (n = the count fn_010927 derived from the header). */
void cd_iso_root_header(uint8_t *hdr7);
void cd_iso_root_records(uint8_t *recs, uint32_t n);

/* One ISO logical sector (2048 bytes) from a disc LBA. 0 = ok. */
int cd_iso_read_sector(uint32_t disc_lba, uint8_t *out2048);

/* One RAW sector (all 2352 bytes, no user-data slice) from a disc LBA -- the Red Book audio
 * tracks are read this way, because fn_010b1d asks MSCDEX for the track's sectors and the
 * original streams them as samples. Measured on TG_NIGHT.IMG: the audio sectors hold
 * little-endian 16-bit signed PCM, 588 stereo frames per sector (see sys.h). 0 = ok. */
int cd_iso_read_audio(uint32_t disc_lba, uint8_t *out2352);

/* ---- an optional REMOTE byte source for the per-track (TRACKS) layout ----
 *
 * The browser host (wasm/) has no filesystem to hold 600 MB of track files and must not download a
 * whole disc to hear one song, so it asks the web server for exactly the bytes a read needs (HTTP
 * Range). The seam is these two callbacks -- "how long is track N", "give me n bytes at an offset" --
 * installed before cd_iso_open(). With them installed the TRACKS layout keeps every piece of its own
 * geometry logic (the parsed TOC, the LBA->track owner search, the lead-out, refusing a read of a
 * track that is not there) and only the fopen/fseek/fread trio is replaced, which is what makes the
 * audit trail and the "absent track is silent, never invented" rule survive the move.
 *
 * Both return 0 on success. A size() that fails means the track is not there at all -- exactly like a
 * missing trackNN.bin -- so the track is bounded by the next INDEX and its reads are refused. A size()
 * that succeeds with *out_bytes == 0 means "present, length unknown" and the CUE's own bound is used. */
typedef int (*cd_iso_size_fn)(uint32_t track, uint64_t *out_bytes);
typedef int (*cd_iso_pread_fn)(uint32_t track, uint64_t byte_off, void *dst, uint32_t n);
void cd_iso_set_remote(cd_iso_size_fn size, cd_iso_pread_fn pread);

#endif

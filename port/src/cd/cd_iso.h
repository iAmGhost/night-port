#ifndef CD_ISO_H
#define CD_ISO_H
#include <stdint.h>

/* Modern CD backend for the port: reads NIGHT's real disc (TG_NIGHT.CUE + TG_NIGHT.IMG) and answers
 * the MSCDEX-style requests fn_010927 (TOC header) and fn_107b9 (TOC entry) would have made.
 * See cd_iso.c for the request/reply decoding and why the disc-present gate fn_010c3e passes on the
 * real pressing. Returns 0 on success, -1 if the cue/img cannot be opened. */
int cd_iso_open(const char *cue_path);
void cd_iso_close(void);

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

#endif

/* of_disc.h — the Pocket-side disc seam: the MSCDEX gate answers from the compiled disc table, and
 * the CD-DA sector reads come from per-track data slots instead of one big .IMG file. */
#ifndef NIGHT_OF_DISC_H
#define NIGHT_OF_DISC_H

#include <stdint.h>

/* Install the sys_cd backend (TOC header + records) built from disc_table.h. Returns the number of
 * tracks the gate will report, 0 when the table is unusable. */
int  of_disc_open(void);

void of_disc_root_header(uint8_t *hdr7);       /* the 7-byte header fn_010927 asks for */
void of_disc_root_records(uint8_t *recs, uint32_t n);  /* n 5-byte fn_107b9 records */

uint32_t of_disc_total_sectors(void);
int      of_disc_track_of_lba(uint32_t disc_lba);   /* track number covering that LBA, -1 if none */
int      of_disc_track_slot(uint32_t track);        /* APF data slot of cd/trackNN.bin, -1 unknown */

/* One 2352-byte raw audio sector of a CDDA track (already little-endian s16 stereo, see
 * src/cd/cd_iso.c's comment and docs/06). Returns 0 on success. */
int  of_disc_read_audio(uint32_t disc_lba, uint8_t *out2352);

#endif /* NIGHT_OF_DISC_H */

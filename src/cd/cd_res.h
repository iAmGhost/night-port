#ifndef NIGHT_CD_RES_H
#define NIGHT_CD_RES_H
#include <stdint.h>

/* NIGHT.RES, read out of the disc the port already has open -- the installer's payload (NIGHT.DAT, an
 * ISO9660 file whose container holds the installed files) found and unpacked here, so a directory with
 * nothing in it but TG_NIGHT.CUE and TG_NIGHT.IMG is a complete copy of the game and needs no extraction
 * step.  The Python reference is scripts/night-extractor (docs/455); this is docs/465.
 *
 * Nothing here is consulted until the resource layer asks for NIGHT.RES and no such file is on disk, so an
 * install set that was already extracted (the extracted/ the rest of this repo assumes) never runs a line
 * of it.  A disc with no image in it answers -1 and the game's own banner follows, as it always did. */

int  cd_res_open(void);                  /* 0 = NIGHT.RES's slot is known; -1 = nothing to read it from */
int  cd_res_found(void);
int  cd_res_size(uint64_t *out);         /* 0 = known; -1 = not found */
int  cd_res_pread(uint64_t off, void *dst, uint32_t n);   /* bytes read, 0 at EOF, -1 with no source */
const char *cd_res_source(void);         /* what was found, for a host that wants to say so */
void cd_res_reset(void);                 /* a different disc: forget what was found */

#endif

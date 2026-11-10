#ifndef CD_AUDIT_H
#define CD_AUDIT_H
#include <stdint.h>

/* A byte-exact gate for the disc read path.

   Why this exists: every audio change since docs/405 has been judged against `cdda.raw`, and docs/410
   established that `cdda.raw` is a wall-clock sampling log of the ring -- three runs of the same binary and
   seed give two different md5s. So there has been no valid way to A/B any change that touches the read path
   (the seven fictional CD-stop sites, the DISC_MIN_WANT clamp, docs/407's causal claim).

   What this is instead: for every read the port asks the disc for, record the offset, the byte count and a
   hash of the bytes that came back. `scripts/pocket/audit_disc.py` then maps each request onto TG_NIGHT.IMG
   through the repack manifest and compares the BYTES. That is a property of the read path and the shipped
   disc, so it reproduces.

   Inert unless cd_audit_open() is given a path. The hash is FNV-1a 64, computed identically in the python
   verifier; on a mismatch the verifier can dump the byte range, so the hash is a locator, not the argument.
*/
void cd_audit_open(const char *path);   /* NULL or "" leaves everything below a no-op */
void cd_audit_read(const char *kind, long long off, uint32_t n, const uint8_t *buf);
void cd_audit_close(void);
#endif

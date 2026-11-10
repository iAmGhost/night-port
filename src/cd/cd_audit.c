#include "cd_audit.h"
#include <stdio.h>
#include <string.h>

static FILE *g_audit;
static uint32_t g_lines;
static uint64_t g_bytes;

/* FNV-1a 64. Mirrored byte for byte in scripts/pocket/audit_disc.py -- if either side changes, the verifier
 * reports every request as a mismatch, which is the loud failure you want. */
static uint64_t fnv1a64(const uint8_t *p, uint32_t n) {
    uint64_t h = 14695981039346656037ull;         /* 0xcbf29ce484222325 */
    for (uint32_t i = 0; i < n; i++) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ull;                    /* 0x100000001b3 */
    }
    return h;
}

void cd_audit_open(const char *path) {
    if (g_audit) return;
    if (!path || !*path) return;
    g_audit = fopen(path, "wb");
    if (!g_audit) return;
    /* Buffered on purpose: this is written from deep inside the game's own spin chain, and a flush per read
     * would cost more than the read. A rig run exits normally and closes the file; a killed run loses the
     * buffer, so cd_audit_close() is part of every rig path. */
    fprintf(g_audit, "# kind\toffset\tnbytes\tfnv1a64\n");
}

void cd_audit_read(const char *kind, long long off, uint32_t n, const uint8_t *buf) {
    if (!g_audit || !buf) return;
    g_lines++;
    g_bytes += n;
    fprintf(g_audit, "%s\t%lld\t%u\t%016llx\n", kind, off, n, (unsigned long long)fnv1a64(buf, n));
}

void cd_audit_close(void) {
    if (!g_audit) return;
    fprintf(g_audit, "# records=%u bytes=%llu\n", g_lines, (unsigned long long)g_bytes);
    fclose(g_audit);
    g_audit = 0;
}

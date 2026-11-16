/* cd_res.c -- the installer's payload, read off the disc it was installed from.  docs/465.
 *
 * WHY THIS EXISTS
 * ---------------
 * The disc does not carry NIGHT.RES.  What it carries is the installer's payload -- one ISO9660 file,
 * NIGHT/NIGHT.DAT -- whose container holds the installed files under the lower-case names SETUP.EXE
 * would have written them in.  So a directory with nothing in it but TG_NIGHT.CUE and TG_NIGHT.IMG is a
 * complete copy of the game, and it used to need an extra step before the port could run it: extract the
 * install set first (scripts/night-extractor, docs/455), then boot.  This file is that step, done at
 * open() time instead: when the resource layer asks for NIGHT.RES and no such file is on disk, the bytes
 * come out of the disc that is already open.
 *
 * The byte geometry is the whole difficulty, so it is crossed in exactly one place.  A MODE1/2352 frame
 * carries 2,048 bytes of user data behind a 16-byte header, so mapping a file's bytes linearly onto the
 * raw image reads everything after the first sector 16 bytes early per sector travelled -- the right size
 * and the wrong bytes, which is the mistake the Python reference recorded when it made it once (docs/455).
 *
 * The Python package is the reference model and this is a field-for-field port of the parts that matter
 * (scripts/night-extractor/night_extractor/disc.py): the root-directory record found by the record's own
 * shape rather than by a remembered table, a breadth-first name lookup that never revisits an extent, and
 * the container index's reverse-XOR obfuscation as the loader fn_0366a4 does it (docs/02-container.md).
 */
#include "cd_res.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "cd_iso.h"

#define UD_LEN 2048u

static int      g_found;                       /* NIGHT.RES's slot is known */
static uint64_t g_off, g_size;                 /* of the payload file, in its own user data */
static uint32_t g_lba;                         /* where the payload file starts, in image sectors */
static char     g_source[128];

static uint32_t rd32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
static uint32_t rd32be(const uint8_t *b) {
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

/* The volume descriptor's own root-directory record, from sectors 16..32, recognised by shape: a record
 * length that agrees with its own identifier length, and the both-endian duplicated pairs for LBA and size
 * agreeing with each other.  TG_NIGHT's descriptor carries "CD001" one byte earlier than the book says,
 * so offsets copied from a table would have read the wrong dwords and still returned bytes; a shape that
 * cannot be a record refuses. */
static int root_record(const uint8_t *d, uint32_t *lba, uint32_t *size, uint32_t cap) {
    for (uint32_t o = 0; o + 34 <= UD_LEN && o < 300; o++) {
        uint32_t L = d[o];
        if (L < 33 || L > 54) continue;
        uint32_t l = rd32(d + o + 2), s = rd32(d + o + 10);
        if (l != rd32be(d + o + 6) || s != rd32be(d + o + 14)) continue;
        if (l == 0 || l >= cap || s == 0 || s > UD_LEN * 64) continue;
        if (d[o + 32] != L - 33) continue;
        *lba = l; *size = s;
        return 1;
    }
    return 0;
}

/* Read one ISO9660 directory extent (which may span sectors) and match `want` against its file records.
 * Directories it passes back for the caller to walk later.  Stops at the first record that cannot be one,
 * so a directory read that runs into file data answers with what it recognised instead of inventing
 * entries. */
static int dir_find(uint32_t lba, uint32_t size, const char *want, uint32_t *out_lba, uint32_t *out_size,
                    uint32_t cap, uint32_t *sub, uint32_t *sub_size, uint32_t *nsub) {
    const uint32_t MAXSUB = 16;
    uint8_t acc[UD_LEN * 2];
    uint32_t ns = (size + UD_LEN - 1) / UD_LEN;
    if (ns == 0 || ns > 64) return 0;               /* a directory of 64 sectors is a misread record */
    for (uint32_t done = 0; done < ns;) {
        uint32_t take = (ns - done > 2) ? 2 : (ns - done);
        for (uint32_t i = 0; i < take; i++)
            if (cd_iso_read_sector(lba + done + i, acc + i * UD_LEN) != 0) return 0;
        for (uint32_t p = 0; p + 34 <= take * UD_LEN;) {
            uint32_t L = acc[p];
            if (L == 0) { p += 1; continue; }
            if (p + L > take * UD_LEN || L < 33) break;
            const uint8_t *e = acc + p;
            uint32_t il = e[32];
            if (33 + il > L) break;
            uint32_t el = rd32(e + 2), es = rd32(e + 10);
            char name[64];
            uint32_t nn = il < sizeof name - 1 ? il : (uint32_t)sizeof name - 1;
            memcpy(name, e + 33, nn); name[nn] = '\0';
            char *semi = strchr(name, ';'); if (semi) *semi = '\0';
            if (e[25] & 2) {                        /* a directory: hand it back unless it is "." or ".." */
                if (il > 1 && el < cap && es <= UD_LEN * 64 && *nsub < MAXSUB) {
                    sub[*nsub] = el; sub_size[*nsub] = es; (*nsub)++;
                }
            } else if (strcasecmp(name, want) == 0) {
                *out_lba = el; *out_size = es;
                return 1;
            }
            p += (L + 1u) & ~1u;
        }
        done += take;
    }
    return 0;
}

/* Breadth-first, case-insensitive, every directory extent visited once.  Both guards are load-bearing:
 * without the self/parent skip and the visited set, a name that is not on the disc re-enters the root
 * through ".." and reads it 2^depth times -- the Python reference hung for four minutes that way before
 * it was interrupted. */
static int iso_find(const char *want, uint32_t cap, uint32_t *lba, uint32_t *size) {
    uint8_t sec[UD_LEN];
    uint32_t rlba = 0, rsize = 0, first = 0;
    for (uint32_t s = 16; s < 33; s++) {
        if (cd_iso_read_sector(s, sec) != 0) continue;
        if (root_record(sec, &rlba, &rsize, cap)) { first = s; break; }
    }
    if (!first) return 0;
    uint32_t queue_lba[32], queue_size[32];
    int head = 0, tail = 0;
    uint32_t seen[64], nseen = 0;
    queue_lba[tail] = rlba; queue_size[tail] = rsize; tail++;
    while (head < tail) {
        uint32_t l = queue_lba[head], sz = queue_size[head];
        head++;
        int dup = 0;
        for (uint32_t i = 0; i < nseen; i++) if (seen[i] == l) { dup = 1; break; }
        if (dup || l >= cap || sz > UD_LEN * 64) continue;
        if (nseen < 64) seen[nseen++] = l;
        uint32_t sub[16], ss[16], nsub = 0;
        if (dir_find(l, sz, want, lba, size, cap, sub, ss, &nsub)) return 1;
        for (uint32_t i = 0; i < nsub && tail < 32; i++) { queue_lba[tail] = sub[i]; queue_size[tail] = ss[i]; tail++; }
    }
    return 0;
}

/* The container's 20-byte-entry index, de-obfuscated the way the loader fn_0366a4 does it (docs/02): a
 * reverse XOR chain over the whole block -- b[i] ^= b[i-1] walking backwards -- then b[0] ^= 0x20. */
static uint32_t container_dir(const uint8_t *raw, uint32_t n,
                              char names[16][16], uint64_t offs[16], uint64_t sizes[16]) {
    uint32_t count = rd32(raw);
    if (count == 0 || count > 16 || 4 + count * 20 > n) return 0;
    uint8_t b[16 * 20];
    memcpy(b, raw + 4, count * 20);
    for (uint32_t i = count * 20 - 1; i > 0; i--) b[i] ^= b[i - 1];
    b[0] ^= 0x20;
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *e = b + i * 20;
        uint32_t j = 0;
        while (j < 12 && e[j] != 0) j++;
        memcpy(names[i], e, j); names[i][j] = '\0';
        sizes[i] = rd32(e + 12);
        offs[i] = rd32(e + 16);
    }
    return count;
}

/* NIGHT.RES's slot, read out of the disc.  Filled in by cd_res_open. */
int cd_res_open(void) {
    if (g_found) return 0;
    if (!cd_iso_image_present()) return -1;
    uint32_t cap = cd_iso_total_sectors();
    uint32_t lba = 0, fsize = 0;
    if (!iso_find("NIGHT/NIGHT.DAT", cap, &lba, &fsize) &&
        !iso_find("NIGHT.DAT", cap, &lba, &fsize))
        return -1;

    /* The payload file's own first sectors: its count, then its directory. */
    uint8_t head[UD_LEN * 2];
    if (cd_iso_read_sector(lba, head) != 0) return -1;
    uint32_t count = rd32(head);
    if (count == 0 || count > 16 || 4 + count * 20 > (uint32_t)sizeof head) return -1;
    char names[16][16];
    uint64_t offs[16], sizes[16];
    uint32_t n = container_dir(head, 4 + count * 20, names, offs, sizes);
    if (n == 0) return -1;
    if (offs[0] != 4 + count * 20) return -1;       /* the index's own consistency test */
    for (uint32_t i = 0; i < n; i++) {
        if (strcasecmp(names[i], "NIGHT.RES") != 0) continue;
        g_off = offs[i]; g_size = sizes[i]; g_lba = lba;
        snprintf(g_source, sizeof g_source, "NIGHT.DAT@%u +%llu (%s, %llu B of %u entries)", lba,
                 (unsigned long long)offs[i], names[i], (unsigned long long)sizes[i], n);
        g_found = 1;
        return 0;
    }
    return -1;
}

const char *cd_res_source(void) { return g_found ? g_source : "none"; }
int cd_res_found(void) { return g_found; }
int cd_res_size(uint64_t *out) { if (!g_found || !out) return -1; *out = g_size; return 0; }

/* `n` bytes of NIGHT.RES at an offset in NIGHT.RES, gathered out of the payload file's user data.  The
 * entry's offset is added here and nowhere else: everything above this line speaks in the payload file's
 * own space, and everything below speaks in NIGHT.RES's. */
int cd_res_pread(uint64_t off, void *dst, uint32_t n) {
    if (!g_found) return -1;
    if (off >= g_size) return 0;                    /* EOF */
    off += g_off;
    uint8_t *p = (uint8_t *)dst;
    uint32_t got = 0;
    while (got < n && off < g_off + g_size) {
        uint32_t sec = (uint32_t)(off / UD_LEN), rem = (uint32_t)(off % UD_LEN);
        uint32_t take = UD_LEN - rem;
        if (take > n - got) take = n - got;
        if (off + take > g_off + g_size) take = (uint32_t)(g_off + g_size - off);
        uint8_t buf[UD_LEN];
        if (cd_iso_read_sector(g_lba + sec, buf) != 0) return got ? (int)got : -1;
        memcpy(p + got, buf + rem, take);
        got += take; off += take;
    }
    return (int)got;
}

void cd_res_reset(void) { g_found = 0; g_off = g_size = 0; g_lba = 0; g_source[0] = '\0'; }

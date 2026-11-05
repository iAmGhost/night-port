/* of_disc.c — the disc seam for the Pocket build.
 *
 * WHAT THIS REPLACES: src/cd/cd_iso.c parses NIGHT/TG_NIGHT.CUE and reads TG_NIGHT.IMG with
 * fseek/fread.  On the Pocket neither file exists in that shape: the audio tracks ship as one APF
 * data slot each (cd/trackNN.bin, slots 18..31, see scripts/pocket/mk_dist.py), and the CUE's layout
 * is compiled into disc_table.h by scripts/pocket/gen_disc.py.
 *
 * WHY THE GATE NEEDS IT AT ALL: fn_010c3e (the disc-present gate) compares the raw dwords at +1 of
 * TOC records 1 and 2 and requires the difference 0xc51 = 3153 sectors; with no backend installed
 * the game sits in the "insert the CD" spin (docs/67).  So the header/record answers are not a
 * nicety, they are the boot path.
 *
 * LBA CONVENTION (same as cd_iso.c): the table's LBAs are MSCDEX disc LBAs (LBA 0 = 2 s of lead-in)
 * and already include the CUE's PREGAP.  A per-track .bin starts at that track's INDEX 01, so the
 * offset inside the slot file is (disc_lba - track_lba) * 2352 -- no -150 here, because the -150 in
 * cd_iso.c exists only to convert a *disc* LBA into the single IMG's file sectors.
 */
#include "of_disc.h"
#include "of_log.h"
#include "sys/sys.h"

#include "disc_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OF_PC
#include "of.h"
#include "of_file.h"
#endif

#define IMG_SECTOR 2352u

/* Track -> slot, mirroring scripts/pocket/mk_dist.py: audio tracks in disc order from slot 18.
 * TG_NIGHT has audio tracks 02..15, so slot = track + 16 (02 -> 18 ... 15 -> 31).  The plan keeps every id
 * in 0..31 and the manifest under Analogue's ceiling of 32 data slots per core (docs/334 §7). */
#define DISC_SLOT_BASE 18
#define DISC_SLOT_TRACK0 2

static int g_have_table;

static const disc_track_t *find_track(uint32_t track) {
    for (uint32_t i = 0; i < DISC_TRACK_COUNT; i++)
        if (g_disc_tracks[i].track == track) return &g_disc_tracks[i];
    return (const disc_track_t *)0;
}

/* The track that owns a disc LBA: the last track whose start is at or below it. */
int of_disc_track_of_lba(uint32_t disc_lba) {
    int best = -1;
    uint32_t best_lba = 0;
    for (uint32_t i = 0; i < DISC_TRACK_COUNT; i++) {
        if (g_disc_tracks[i].lba <= disc_lba && (best < 0 || g_disc_tracks[i].lba >= best_lba)) {
            best = (int)g_disc_tracks[i].track;
            best_lba = g_disc_tracks[i].lba;
        }
    }
    return best;
}

int of_disc_track_slot(uint32_t track) {
    const disc_track_t *t = find_track(track);
    if (!t || !t->audio) return -1;
    if (track < DISC_SLOT_TRACK0) return -1;
    int slot = DISC_SLOT_BASE + (int)(track - DISC_SLOT_TRACK0);
    if (slot > 63) return -1;
    return slot;
}

uint32_t of_disc_total_sectors(void) {
#ifdef DISC_TOTAL_SECTORS
    return DISC_TOTAL_SECTORS;
#else
    /* No IMG at generation time: fall back to the last track's extent, which is what the game's
     * own watchdogs need (it never reads the lead-out for anything but the gate). */
    return g_disc_tracks[DISC_TRACK_COUNT - 1].lba;
#endif
}

void of_disc_root_header(uint8_t *hdr7) {
    if (!hdr7) return;
#ifdef DISC_LEADOUT_M
    hdr7[0] = 0x0a;                    /* drive number, the ROM seed value (cd_iso.c) */
    hdr7[1] = (uint8_t)DISC_FIRST_TRACK;
    hdr7[2] = (uint8_t)DISC_LAST_TRACK;
    hdr7[3] = DISC_LEADOUT_F;          /* lead-out MSF, in the same byte order MSCDEX used */
    hdr7[4] = DISC_LEADOUT_S;
    hdr7[5] = DISC_LEADOUT_M;
    hdr7[6] = 0;
#else
    memset(hdr7, 0, 7);
#endif
}

void of_disc_root_records(uint8_t *recs, uint32_t n) {
    if (!recs) return;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t tr = DISC_FIRST_TRACK + i;
        const disc_track_t *t = find_track(tr);
        uint32_t lba = t ? t->lba : 0u;
        recs[i * 5 + 0] = (uint8_t)tr;
        /* The shipped record layout is unaligned (BYTE then DWORD); copy bytes instead of casting a
         * dword in, which is what src/cd/cd_iso.c does with its ua32 view. */
        recs[i * 5 + 1] = (uint8_t)(lba & 0xffu);
        recs[i * 5 + 2] = (uint8_t)((lba >> 8) & 0xffu);
        recs[i * 5 + 3] = (uint8_t)((lba >> 16) & 0xffu);
        recs[i * 5 + 4] = (uint8_t)((lba >> 24) & 0xffu);
    }
}

int of_disc_open(void) {
    g_have_table = (DISC_TRACK_COUNT > 0);
    if (!g_have_table) return 0;
    sys_cd_set_backend(of_disc_root_header, of_disc_root_records);
    return (int)DISC_TRACK_COUNT;
}

int of_disc_read_audio(uint32_t disc_lba, uint8_t *out2352) {
    if (!out2352) return -1;
    int track = of_disc_track_of_lba(disc_lba);
    const disc_track_t *t = track > 0 ? find_track((uint32_t)track) : (const disc_track_t *)0;
    if (!t || !t->audio) return -1;
    uint32_t off_sectors = disc_lba - t->lba;

#ifndef OF_PC
    /* "slot:N" is the SDK's documented direct slot handle (README "File I/O"): stdio over the data slot, the
     * same read path fopen("NIGHT.RES") uses.  A 4 s prefill is 300 sector reads, and the old code opened and
     * closed the slot for every one of them -- 300 SD opens for 705 KB, and a failure anywhere in that run
     * ended the prefill ("cdda refused: prefill short (111 of 300 sectors)") with nothing said about which
     * sector, which slot, or how much arrived.  One handle per track, seek only when the read is not the next
     * sector in order, and a refusal that names the slot, the sector and the byte count. */
    static FILE *af;
    static int   a_slot = -1;
    static long  a_next = -1;
    int slot = of_disc_track_slot(t->track);
    if (slot < 0) return -1;
    if (a_slot != slot) {
        if (af) { fclose(af); af = 0; }
        char name[16];
        snprintf(name, sizeof name, "slot:%d", slot);
        af = fopen(name, "rb");
        if (!af) { a_slot = -1; a_next = -1; of_log_line("disc: fopen \"%s\" failed\n", name); return -1; }
        a_slot = slot; a_next = -1;
    }
    long off = (long)off_sectors * IMG_SECTOR;
    if (a_next != off) {
        if (fseek(af, off, SEEK_SET) != 0) {
            of_log_line("disc: seek slot %d to sector %u failed\n", slot, off_sectors);
            fclose(af); af = 0; a_slot = -1; a_next = -1; return -1;
        }
    }
    size_t got = fread(out2352, 1, IMG_SECTOR, af);
    if (got != IMG_SECTOR) {
        of_log_line("disc: slot %d sector %u read %u of %u bytes\n", slot, off_sectors,
                    (unsigned)got, (unsigned)IMG_SECTOR);
        fclose(af); af = 0; a_slot = -1; a_next = -1;
        return -1;
    }
    a_next = off + IMG_SECTOR;
    return 0;
#else
    /* Desktop build: read the same per-track files from a staging directory.
     *
     * The directory is resolved once into a static buffer: this runs deep inside the game's own call
     * chain (spin -> tick pump -> cdda pump -> here), and the run should not lean on a libc-owned
     * buffer across that depth.  (The fault this file did cause was of a different kind -- see
     * docs/331 §7: getenv without <stdlib.h> is an implicit int, which truncates the pointer.) */
    static char dir_buf[512];
    static int dir_ready;
    if (!dir_ready) {
        const char *e = getenv("NIGHT_CD_DIR");
        snprintf(dir_buf, sizeof dir_buf, "%s", (e && *e) ? e : "scratch/pocket/cd");
        dir_ready = 1;
        printf("of_disc: desktop cd dir = %s\n", dir_buf);
    }
    char path[768];
    snprintf(path, sizeof path, "%s/track%02u.bin", dir_buf, t->track);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, (long)off_sectors * IMG_SECTOR, SEEK_SET) != 0) { fclose(f); return -1; }
    size_t got = fread(out2352, 1, IMG_SECTOR, f);
    fclose(f);
    return got == IMG_SECTOR ? 0 : -1;
#endif
}

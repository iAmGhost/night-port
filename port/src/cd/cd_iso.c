/* cd_iso.c -- the modern CD backend: TG_NIGHT.CUE / TG_NIGHT.IMG -> the MSCDEX answers NIGHT asks.
 *
 * WHAT NIGHT ACTUALLY ASKS FOR (resolved against the real disc; this closes the open question in
 * docs/28-disc-present-gate.md).
 *
 * fn_010927 is not a root-directory read, it is the MSCDEX **read TOC header** request (code 3 in the
 * 26-byte request packet copied from ROM 0x51ad4: {len=0x1a, unit=0, code=3}). The 7-byte reply that
 * the game copies in/out of DAT_00053c5c is:
 *
 *   +0 drive/unit number          (ROM seed 0x0a)
 *   +1 first track number         -> bufA[1]
 *   +2 last track number          -> bufA[2]   => DAT_00053c60 = last - first + 1 = 15 = 0xf  (TRACK 01..15
 *                                             of TG_NIGHT.CUE: 1 MODE1 data + 14 audio)
 *   +3..+5 lead-out address, MSF  -> the game's own MSF->LBA conversion
 *                                   bufA[3] + bufA[4]*75 + bufA[5]*6420 - 150  (the 6420 coefficient is
 *                                   the original's bug: minutes should cost 4500 sectors; ported as-is)
 *
 * fn_107b9 (called once per entry, i == first..last) is the MSCDEX **read TOC entry** request (code 4)
 * and fills the 5-byte records the game allocates at DAT_00053c64: {BYTE trackNumber; DWORD startLBA}.
 * That stride is why the disc-present gate fn_010c3e reads them at +6 and +0xb -- entry 1 and entry 2
 * (tracks 2 and 3) -- and requires their start LBAs to differ by 0xc51 = 3153 sectors. On the real disc
 * (TG_NIGHT.CUE, with the PREGAP 00:02:00 in front of TRACK 02 shifting every later track 150 sectors):
 *
 *   TRACK 02  index 08:14:62 (+pregap)  -> disc LBA 37112
 *   TRACK 03  index 08:56:65 (+pregap)  -> disc LBA 40265      40265 - 37112 = 3153 = 0xc51  PASS
 *
 * i.e. the gate is a TOC-signature check of the original pressing, and the real disc satisfies it.
 *
 * IMAGE FORM: TG_NIGHT.IMG is BINARY MODE1/2352 (CUE: FILE "TG_NIGHT.IMG" BINARY / TRACK 01
 * MODE1/2352). Its 2352-byte sectors carry the ISO9660 user data at offset 16 (the PVD signature
 * "CD001" sits at 16 + 1, and the ISO root directory record decodes from the PVD at +156), so a
 * logical sector read is [16, 16+2048) of the sector.
 */
#include "cd_iso.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMG_SECTOR 2352
#define IMG_DATA_OFF 16
#define IMG_DATA_LEN 2048

typedef struct { uint32_t track; int32_t lba; } cd_toc_entry;  /* lba = disc LBA, pregap-corrected */

static cd_toc_entry g_toc[100];
static uint32_t     g_toc_n;
static uint32_t     g_first_track, g_last_track;
static uint8_t      g_leadout_msf[3];     /* lead-out in disc MSF */
static uint32_t     g_total_sectors;
static FILE        *g_img;
static char         g_img_path[1024];
static int          g_opened;

/* ---- MSF <-> disc LBA (150-sector lead-in) ---- */
static uint32_t msf_to_lba(uint32_t m, uint32_t s, uint32_t f) {
    return m * 4500u + s * 75u + f;               /* frames from disc start (MSF address) */
}

/* ---- tiny CUE parser: FILE / TRACK / INDEX / PREGAP ---- */
static int cue_time(const char *t, uint32_t *m, uint32_t *s, uint32_t *f) {
    return sscanf(t, "%u:%u:%u", m, s, f) == 3;
}

int cd_iso_open(const char *cue_path) {
    char dir[1024];
    char cue_img[1024];
    FILE *fp = fopen(cue_path, "rb");
    if (!fp) return -1;

    /* directory of the cue, for resolving the FILE name */
    snprintf(dir, sizeof dir, "%s", cue_path);
    char *slash = strrchr(dir, '/');
    if (slash) *slash = '\0'; else snprintf(dir, sizeof dir, ".");

    char line[2048];
    uint32_t pregap_frames = 0;          /* accumulated virtual silence (PREGAP) before tracks */
    uint32_t cur_track = 0;
    uint32_t track_first_frame = 0;      /* file position of the current track's first index */
    int      have_first_index = 0;
    uint32_t img_sectors = 0;

    g_toc_n = 0; cur_track = 0; have_first_index = 0;

    while (fgets(line, sizeof line, fp)) {
        char kw[64];
        if (sscanf(line, " %63s", kw) != 1) continue;   /* leading space: the cue indents its directives */
        if (strcmp(kw, "FILE") == 0) {
            char name[1024] = {0};
            char fmt[64] = {0};
            if (sscanf(line, " FILE \"%1023[^\"]\" %63s", name, fmt) >= 1)
                snprintf(cue_img, sizeof cue_img, "%s/%s", dir, name);
        } else if (strcmp(kw, "TRACK") == 0) {
            unsigned tn = 0; char tf[32] = {0};
            if (sscanf(line, " TRACK %u %31s", &tn, tf) == 2) {
                if (cur_track && have_first_index) {   /* close the previous track's start */
                    if (g_toc_n < 100) {
                        g_toc[g_toc_n].track = cur_track;
                        g_toc[g_toc_n].lba = (int32_t)(msf_to_lba(0,0,track_first_frame) + pregap_frames);
                        g_toc_n++;
                    }
                }
                cur_track = tn; have_first_index = 0; track_first_frame = 0;
                if (g_toc_n == 0) g_first_track = tn;
                g_last_track = tn;
            }
        } else if (strcmp(kw, "PREGAP") == 0) {
            char t[32] = {0};
            if (sscanf(line, " PREGAP %31s", t) == 1) {
                uint32_t m, s, f;
                if (cue_time(t, &m, &s, &f)) pregap_frames += msf_to_lba(m, s, f);
            }
        } else if (strcmp(kw, "INDEX") == 0) {
            unsigned in = 0; char t[32] = {0};
            if (sscanf(line, " INDEX %u %31s", &in, t) == 2 && in == 1 && !have_first_index) {
                uint32_t m, s, f;
                if (cue_time(t, &m, &s, &f)) {
                    track_first_frame = msf_to_lba(m, s, f);
                    have_first_index = 1;
                }
            }
        }
    }
    if (cur_track && have_first_index && g_toc_n < 100) {   /* close the last track */
        g_toc[g_toc_n].track = cur_track;
        g_toc[g_toc_n].lba = (int32_t)(track_first_frame + pregap_frames);
        g_toc_n++;
    }
    fclose(fp);
    if (g_toc_n == 0) return -1;

    g_img = fopen(cue_img, "rb");
    if (!g_img) return -1;
    snprintf(g_img_path, sizeof g_img_path, "%s", cue_img);
    long sz = ftell(g_img);
    fseek(g_img, 0, SEEK_END);
    sz = ftell(g_img);
    img_sectors = (uint32_t)(sz / IMG_SECTOR);

    /* The image holds the *file* frame count; the disc has pregap_frames of virtual silence inside,
     * so the lead-out (end of the last track) sits at file sectors + pregap. */
    g_total_sectors = img_sectors + pregap_frames;
    {
        uint32_t addr = g_total_sectors + 150u;          /* disc MSF address of the lead-out */
        g_leadout_msf[2] = (uint8_t)(addr % 75u);
        uint32_t sec = addr / 75u;
        g_leadout_msf[1] = (uint8_t)(sec % 60u);
        g_leadout_msf[0] = (uint8_t)(sec / 60u);
    }
    g_opened = 1;
    return 0;
}

uint32_t cd_iso_track_count(void)    { return g_toc_n; }
uint32_t cd_iso_first_track(void)    { return g_first_track; }
uint32_t cd_iso_last_track(void)     { return g_last_track; }
uint32_t cd_iso_total_sectors(void)  { return g_total_sectors; }
int32_t  cd_iso_track_lba(uint32_t track) {
    for (uint32_t i = 0; i < g_toc_n; i++) if (g_toc[i].track == track) return g_toc[i].lba;
    return -1;
}

/* ---- the sys_cd seam: exactly what fn_010927 / fn_107b9 would have got from MSCDEX ---- */
void cd_iso_root_header(uint8_t *hdr7) {
    hdr7[0] = 0x0a;                     /* drive number (the ROM seed value) */
    hdr7[1] = (uint8_t)g_first_track;   /* first track */
    hdr7[2] = (uint8_t)g_last_track;    /* last track -> DAT_00053c60 = last-first+1 */
    hdr7[3] = g_leadout_msf[2];         /* lead-out frames */
    hdr7[4] = g_leadout_msf[1];         /* lead-out seconds */
    hdr7[5] = g_leadout_msf[0];         /* lead-out minutes */
    hdr7[6] = 0;
}

void cd_iso_root_records(uint8_t *recs, uint32_t n) {
    /* 5-byte MSCDEX TOC records: {BYTE trackNumber; DWORD startLBA} -- entry i is the i-th track
     * between first and last, in disc order. */
    for (uint32_t i = 0; i < n; i++) {
        uint32_t tr = g_first_track + i;
        int32_t lba = cd_iso_track_lba(tr);
        recs[i * 5 + 0] = (uint8_t)tr;
        *(uint32_t *)(recs + i * 5 + 1) = (uint32_t)lba;
    }
}

/* ---- logical sector read (MODE1/2352, user data at +16) ---- */
int cd_iso_read_sector(uint32_t disc_lba, uint8_t *out2048) {
    if (!g_opened || !out2048) return -1;
    if (disc_lba >= g_total_sectors) return -1;
    if (fseek(g_img, (long)disc_lba * IMG_SECTOR + IMG_DATA_OFF, SEEK_SET) != 0) return -1;
    if (fread(out2048, 1, IMG_DATA_LEN, g_img) != IMG_DATA_LEN) return -1;
    return 0;
}

void cd_iso_close(void) {
    if (g_img) fclose(g_img);
    g_img = 0; g_opened = 0; g_toc_n = 0;
}

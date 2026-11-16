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
 *
 * TWO BYTE SOURCES, ONE TOC (the second one is what lets the port run without the 633 MB image):
 *
 *   IMG     the single raw MODE1/2352 image the CUE's FILE line names. A disc LBA is (lba - 150) * 2352
 *           in that file, because the image carries no PREGAP. This is the original behaviour and stays
 *           the default, so every existing comparison basis (scripts/pocket/audit_disc.py, the fixture
 *           suite, the docs/419 asset audit) keeps measuring the same thing.
 *
 *   TRACKS  one file per CD-DA track -- cd/trackNN.bin, raw 2352-byte frames, no header -- i.e. exactly
 *           the files scripts/pocket/repack.py cuts for the Pocket's APF data slots and scripts/
 *           extract_disc.py cuts into extracted/. A file starts at its own track's INDEX 01, so the
 *           offset inside it is (disc_lba - track_lba) * 2352: no -150, because the -150 exists only to
 *           convert a *disc* LBA into one big image's file sectors (pocket/src/night/of_disc.c makes the
 *           same point about its slots). The bytes both sources return for the same LBA are equal by
 *           construction, and the fixture suite checks that on the real disc.
 *
 * Which one answers is decided in cd_iso_open(): an explicit NIGHT_CD_DIR wins (so a rig can A/B them),
 * otherwise the IMG is used, and only when the IMG cannot be opened does the directory beside the CUE
 * get looked for. That ordering makes `./build/night --res-dir extracted` work with no image present.
 *
 * A THIRD CASE, AND WHY THE GATE IS BYPASSABLE: the game's disc-present gate fn_010c3e polls the TOC
 * until it sees 15 tracks whose track-3-minus-track-2 gap is 0xc51, so parsing a CUE used to be a START
 * requirement -- hand the port a tree with nothing but its resources and it loops on cdinsert.pcx
 * forever. When there is no CUE to parse (or NIGHT_NO_DISC asks for it), the geometry comes from
 * src/gen/disc_toc.h, the same numbers the CUE states, generated once from the real disc by
 * scripts/gen/gen_disc_toc.py. The gate then passes, the game runs, and every read_audio is refused --
 * the TOC is borrowed, the samples are not invented. NIGHT_REQUIRE_DISC=1 makes a missing CUE fatal
 * again, which is what an audit run that means to compare against the shipped image should set.
 *
 * A track file is optional in the TRACKS layout too: a missing one is recorded and refused per read, so
 * a partial tree plays the music it has instead of losing the disc, and cd_iso_tracks_missing() lets
 * the host say so at boot.
 */
#include "cd_iso.h"
#include "cd_audit.h"
#include "gen/disc_toc.h"        /* the compiled-in geometry: scripts/gen/gen_disc_toc.py, docs/455 */

/* ---- unaligned flat-model views (docs/126) ----------------------------------
 * The shipped record layout is packed and deliberately unaligned (dword stores at +0x01/+0x05/+0x17 ...
 * read out of the entity constructor fn_010efc, docs/125 S1), so every typed access into it is a misaligned
 * access in C terms.  A packed struct view is alignment-safe on gcc/clang and keeps the exact store width
 * the shipped instruction used; scripts/port/unaligned_casts.py produces the rewrite.
 * The packed attribute is what makes the view safe -- without it the ABI (and UBSan) still require the
 * member's natural alignment, which is exactly what the shipped layout violates. */
#if defined(__GNUC__)
#  define UA_ATTR __attribute__((packed, aligned(1)))
#else
#  define UA_ATTR
#endif
struct UA_ATTR ua32 { uint32_t v; };

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMG_SECTOR 2352
#define IMG_DATA_OFF 16
#define IMG_DATA_LEN 2048

typedef struct { uint32_t track; int32_t lba; uint8_t audio; } cd_toc_entry;  /* lba = disc LBA, pregap-corrected */

static cd_toc_entry g_toc[100];
static uint32_t     g_toc_n;
static uint32_t     g_first_track, g_last_track;
static uint8_t      g_leadout_msf[3];     /* lead-out in disc MSF */
static uint32_t     g_total_sectors;
static FILE        *g_img;
static char         g_img_path[1024];
static int          g_opened;

/* The TRACKS byte source: g_cd_dir is non-empty, g_sectors[i] is the frame count of track g_toc[i] as
 * its file says, and the read handle is kept for the track most recently asked about (the reader walks
 * a track forward sector by sector, and re-opening per sector was measured to be the whole cost of a
 * prefill on the Pocket -- pocket/src/night/of_disc.c). */
static char         g_cd_dir[1024];
static uint32_t     g_sectors[100];
static uint8_t      g_track_absent[100];    /* 1 = no trackNN.bin for this entry: the read is refused */
static int          g_tracks_missing;       /* ... counted, so the host can report them at boot */
static int          g_tracks_total;
static int          g_no_disc;              /* 1 = the TOC is the compiled-in table, nothing was parsed */
static int          g_require_disc;         /* NIGHT_REQUIRE_DISC: the stricter switch, and it wins */
static int          g_no_disc_wanted;       /* NIGHT_NO_DISC was set -- reported when it was ignored */
static FILE        *g_tf;
static uint32_t     g_tf_track;
static long         g_tf_next;

/* The REMOTE byte source (cd_iso.h): when g_remote_pread is installed, the audio frames of a track
 * come from these callbacks instead of the trackNN.bin FILE*, and g_track_absent says "the remote
 * could not name the track", not "there was no file". Installed by the browser host only; every other
 * host leaves both NULL and the code below takes exactly the branch it always took. */
static cd_iso_size_fn  g_remote_size;
static cd_iso_pread_fn g_remote_pread;

void cd_iso_set_remote(cd_iso_size_fn size, cd_iso_pread_fn pread) {
    g_remote_size = size; g_remote_pread = pread;
}

/* ---- MSF <-> disc LBA (150-sector lead-in) ---- */
static uint32_t msf_to_lba(uint32_t m, uint32_t s, uint32_t f) {
    return m * 4500u + s * 75u + f;               /* frames from disc start (MSF address) */
}

/* ---- tiny CUE parser: FILE / TRACK / INDEX / PREGAP ---- */
static int cue_time(const char *t, uint32_t *m, uint32_t *s, uint32_t *f) {
    return sscanf(t, "%u:%u:%u", m, s, f) == 3;
}

/* defined after cd_iso_open, where the TOC they act on has been parsed */
static int  cd_finish(void);
static int  cd_open_tracks(void);

/* The compiled-in geometry, for a boot with no CUE to parse: the numbers, and no byte source. */
static int cd_load_virtual_toc(void) {
    if (DISC_TOC_TRACKS > 100) return -1;
    g_toc_n = 0;
    for (uint32_t i = 0; i < DISC_TOC_TRACKS; i++) {
        g_toc[i].track = g_disc_toc_track[i];
        g_toc[i].lba   = (int32_t)g_disc_toc_lba[i];
        g_toc[i].audio = g_disc_toc_audio[i];
        g_sectors[i]   = 0;                  /* no file: no claim about the track's length */
        g_track_absent[i] = 1;
    }
    g_toc_n = DISC_TOC_TRACKS;
    g_first_track = DISC_TOC_FIRST;
    g_last_track  = DISC_TOC_LAST;
    g_total_sectors = DISC_TOC_TOTAL_SECTORS;
    g_cd_dir[0] = '\0';
    g_tracks_total = 0;
    for (uint32_t i = 0; i < DISC_TOC_TRACKS; i++) if (g_disc_toc_audio[i]) g_tracks_total++;
    g_tracks_missing = g_tracks_total;       /* every one of them */
    g_no_disc = 1;
    return cd_finish();
}

static int  cd_owner(uint32_t disc_lba);

/* The compiled-in geometry, as the cue sheet it was generated from.  scripts/gen/gen_disc_toc.py reads a
 * CUE and keeps the disc LBAs; a host that has a byte source but no CUE on disk -- the browser's
 * bring-your-own-data run (docs/464), where the visitor's file may carry only a fan repack's cue, which
 * describes the fan image and not this disc -- needs the sheet back, and this is the only place that
 * knows both forms.  Asserted against extracted/TG_NIGHT.CUE by a fixture, because a rendering of the
 * geometry that drifts from the geometry would be a silent music mismatch. */
int cd_iso_toc_cue(char *dst, uint32_t cap)
{
    uint32_t n = 0, first_audio = DISC_TOC_TRACKS, pregap = DISC_TOC_PREGAP;
    int rc;
    for (uint32_t i = 0; i < DISC_TOC_TRACKS; i++)
        if (g_disc_toc_audio[i]) { first_audio = i; break; }
#define CUE(...) do { rc = snprintf(dst + n, (size_t)(cap - n), __VA_ARGS__);                     \
                      if (rc < 0 || (uint32_t)rc >= cap - n) return -1; n += (uint32_t)rc; } while (0)
    CUE("FILE \"TG_NIGHT.IMG\" BINARY\r\n");
    for (uint32_t i = 0; i < DISC_TOC_TRACKS; i++) {
        /* One gap on this disc, the PREGAP before the first audio track: the image is everything after
         * it, so every track from there on sits DISC_TOC_PREGAP sectors earlier in the image than the
         * disc counts it (which is exactly what scripts/gen/gen_disc_toc.py did going the other way). */
        uint32_t img = (i >= first_audio) ? g_disc_toc_lba[i] - DISC_TOC_PREGAP : g_disc_toc_lba[i];
        CUE("  TRACK %02u %s\r\n", g_disc_toc_track[i],
            g_disc_toc_audio[i] ? "AUDIO" : "MODE1/2352");
        if (i == first_audio && pregap)
            CUE("    PREGAP %02u:%02u:%02u\r\n", pregap / 4500,
                (pregap / 75) % 60, pregap % 75);
        CUE("    INDEX 01 %02u:%02u:%02u\r\n", img / 4500, (img / 75) % 60, img % 75);
    }
#undef CUE
    return (int)n;
}

int cd_iso_open(const char *cue_path) {
    char dir[1024];
    char cue_img[1024];
    g_no_disc = 0;
    g_require_disc = g_no_disc_wanted = 0;
    g_tracks_missing = g_tracks_total = 0;
    for (uint32_t i = 0; i < 100; i++) g_track_absent[i] = 0;

    /* The two switches answer different questions, so they compose instead of competing:
     *
     *   NIGHT_NO_DISC     -- is there a disc to look at?  Set, the drive is treated as EMPTY: the CUE,
     *                        the image and the track files are not opened even when they are sitting
     *                        there. This is how a run measures the no-disc path with the tree in place.
     *   NIGHT_REQUIRE_DISC -- and may we paper over its absence?  Set, nothing is substituted, so
     *                        fn_010c3e sees no TOC and shows cdinsert.pcx -- the original's own answer
     *                        to an empty drive.
     *
     * So NO_DISC alone runs the game silent on the compiled-in TOC (the shipped-to-a-SSD case), and
     * NO_DISC + REQUIRE is the authentic insert-CD screen with the files still on disk. Nothing here
     * silently ignores the other switch, and the host logs which combination it is in. */
    const char *need = getenv("NIGHT_REQUIRE_DISC");
    g_require_disc = (need != 0 && need[0] != '\0' && strcmp(need, "0") != 0);
    const char *env = getenv("NIGHT_NO_DISC");
    g_no_disc_wanted = (env != 0 && env[0] != '\0' && strcmp(env, "0") != 0);
    if (g_no_disc_wanted) return g_require_disc ? -1 : cd_load_virtual_toc();

    FILE *fp = fopen(cue_path, "rb");
    if (!fp) {
        if (g_require_disc) return -1;      /* asked for a disc, got none: the gate has to say so */
        return cd_load_virtual_toc();
    }

    /* directory of the cue, for resolving the FILE name */
    snprintf(dir, sizeof dir, "%s", cue_path);
    char *slash = strrchr(dir, '/');
    if (slash) *slash = '\0'; else snprintf(dir, sizeof dir, ".");

    char line[2048];
    uint32_t pregap_frames = 0;          /* accumulated virtual silence (PREGAP) before tracks */
    uint32_t cur_track = 0;
    uint32_t track_first_frame = 0;      /* file position of the current track's first index */
    int      have_first_index = 0;
    int      cur_audio = 0;
    uint32_t img_sectors = 0;

    g_toc_n = 0; cur_track = 0; have_first_index = 0;
    g_cd_dir[0] = '\0';
    if (g_img) { fclose(g_img); g_img = 0; }          /* a re-open must not leak the previous source */
    if (g_tf)  { fclose(g_tf);  g_tf  = 0; }
    g_tf_track = 0; g_tf_next = -1;
    for (uint32_t i = 0; i < 100; i++) g_sectors[i] = 0;

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
                        g_toc[g_toc_n].audio = cur_audio;
                        g_toc_n++;
                    }
                }
                cur_track = tn; have_first_index = 0; track_first_frame = 0;
                cur_audio = (strncmp(tf, "AUDIO", 5) == 0);   /* the only mode here with a per-track file */
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
        g_toc[g_toc_n].audio = cur_audio;
        g_toc_n++;
    }
    fclose(fp);
    if (g_toc_n == 0) return -1;

    /* ---- which byte source answers the reads (see the file comment) ----
     * NIGHT_CD_DIR is the one knob: given, it must work (a silent fallback to the image would let a rig
     * measure the source it did not ask for). Not given, the image named by the CUE answers if it can be
     * opened -- today's behaviour, unchanged -- and only a missing image sends us looking for per-track
     * files beside the CUE, which is what makes `--res-dir extracted` run with nothing but 669 bytes of
     * text and 570 MB of track files. */
    env = getenv("NIGHT_CD_DIR");
    if (env != 0 && env[0] != '\0') {
        snprintf(g_cd_dir, sizeof g_cd_dir, "%s", env);
        /* Asked for and not found -- not even one track file -- stays an error: a rig that named
         * TRACKS must not measure the image, or silence, without saying so. */
        if (cd_open_tracks() != 0 || g_tracks_missing == g_tracks_total) {
            g_cd_dir[0] = '\0'; g_tracks_missing = g_tracks_total = 0;
            return -1;
        }
        return cd_finish();
    }
    g_img = fopen(cue_img, "rb");
    if (g_img) {
        snprintf(g_img_path, sizeof g_img_path, "%s", cue_img);
        fseek(g_img, 0, SEEK_END);
        img_sectors = (uint32_t)(ftell(g_img) / IMG_SECTOR);
    } else {
        snprintf(g_cd_dir, sizeof g_cd_dir, "%s/cd", dir);      /* extracted/ and scratch/pocket/ layout */
        if (cd_open_tracks() != 0 || g_tracks_missing == g_tracks_total) {
            snprintf(g_cd_dir, sizeof g_cd_dir, "%s/cdda", dir);
            if (cd_open_tracks() != 0 || g_tracks_missing == g_tracks_total) g_cd_dir[0] = '\0';
        }
        return cd_finish();       /* with no byte source the reads are refused: the game runs silent */
    }

    /* The image holds the *file* frame count; the disc has pregap_frames of virtual silence inside,
     * so the lead-out (end of the last track) sits at file sectors + pregap. */
    g_total_sectors = img_sectors + pregap_frames;
    return cd_finish();
}

/* Lead-out MSF from the total sector count, in the byte order MSCDEX returned. Shared by both sources:
 * the game's own MSF->LBA conversion (with its 6420-per-minute bug) reads these three bytes. */
static int cd_finish(void) {
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

/* Open g_cd_dir as the byte source. A track file may be PRESENT or ABSENT; what it cannot be is
 * malformed, so a file that is not a whole number of 2352-byte frames still refuses the whole tree --
 * that is a half-cut extraction, and playing half a track is the bug this used to prevent.
 *
 * The frame counts come from the files where they exist, because a file is the only claim about a
 * track's end this layout carries for the LAST track (the CUE's last INDEX has nothing after it to
 * bound it); a missing file's track is bounded by the next track's INDEX, which the CUE does state, and
 * reads inside it are refused (see g_track_absent). So a tree with all 14 files answers the full disc,
 * a tree with some plays the music it has, and a tree with none is refused as a byte source by the
 * caller -- which leaves the game running on the TOC alone. On the complete tree the total is the last
 * track's extent, which equals img sectors + pregap, so the lead-out the game reads is the same number
 * from either source. */
static int cd_open_tracks(void) {
    char path[1200];
    uint32_t total = 0;
    int audio_n = 0, missing = 0;

    if (g_cd_dir[0] == '\0') return -1;
    for (uint32_t i = 0; i < g_toc_n; i++) {
        g_sectors[i] = 0;
        g_track_absent[i] = 0;
        if (!g_toc[i].audio) continue;
        audio_n++;
        snprintf(path, sizeof path, "%s/track%02u.bin", g_cd_dir, g_toc[i].track);
        uint32_t n = 0;
        if (g_remote_size) {
            /* The remote answers the same question a stat would: "how many bytes is this track".
             * 0 bytes means "present, length unknown" (a server that refuses HEAD), and the CUE's
             * next INDEX bounds it below -- but it stays PRESENT, because a read may still succeed. */
            uint64_t bytes = 0;
            if (g_remote_size(g_toc[i].track, &bytes) != 0) { missing++; g_track_absent[i] = 1; }
            else if (bytes && (bytes < IMG_SECTOR || bytes % IMG_SECTOR)) return -1;   /* malformed */
            else n = (uint32_t)(bytes / IMG_SECTOR);
            if (n == 0 && i + 1 < g_toc_n && g_toc[i + 1].lba > g_toc[i].lba)
                n = (uint32_t)(g_toc[i + 1].lba - g_toc[i].lba);   /* the next INDEX bounds it */
        } else {
            FILE *f = fopen(path, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long sz = ftell(f);
                fclose(f);
                if (sz < (long)IMG_SECTOR || sz % IMG_SECTOR) return -1;
                n = (uint32_t)(sz / IMG_SECTOR);
            } else {
                missing++;
                g_track_absent[i] = 1;
                if (i + 1 < g_toc_n && g_toc[i + 1].lba > g_toc[i].lba)
                    n = (uint32_t)(g_toc[i + 1].lba - g_toc[i].lba);   /* the next INDEX bounds it */
            }
        }
        if (i + 1 < g_toc_n) {                       /* fits between its own INDEX and the next one */
            int32_t span = g_toc[i + 1].lba - g_toc[i].lba;
            if (span > 0 && n > (uint32_t)span) return -1;
        }
        g_sectors[i] = n;
        if (g_toc[i].lba + (int32_t)n > (int32_t)total) total = (uint32_t)(g_toc[i].lba + (int32_t)n);
    }
    if (audio_n == 0) return -1;
    g_tracks_missing = missing;
    g_tracks_total = audio_n;
    g_total_sectors = total;
    return 0;
}

/* The AUDIO track that owns a disc LBA: the last one whose INDEX 01 is at or below it. The MODE1 track
 * owns its LBAs but has no file here, so it is deliberately not a candidate. */
static int cd_owner(uint32_t disc_lba) {
    int best = -1;
    int32_t best_lba = -1;
    for (uint32_t i = 0; i < g_toc_n; i++) {
        if (!g_toc[i].audio || g_toc[i].lba < 0) continue;
        if ((uint32_t)g_toc[i].lba <= disc_lba && g_toc[i].lba >= best_lba) { best = (int)i; best_lba = g_toc[i].lba; }
    }
    return best;
}

uint32_t cd_iso_track_count(void)    { return g_toc_n; }
uint32_t cd_iso_first_track(void)    { return g_first_track; }
uint32_t cd_iso_last_track(void)     { return g_last_track; }
uint32_t cd_iso_total_sectors(void)  { return g_total_sectors; }
const char *cd_iso_source(void)      { return g_cd_dir[0] ? g_cd_dir : (g_img ? g_img_path : "none"); }
int cd_iso_no_disc(void)             { return g_no_disc; }
int cd_iso_require_disc(void)        { return g_require_disc; }
int cd_iso_disc_absent(void)         { return g_no_disc || g_no_disc_wanted; }
int cd_iso_no_disc_wanted(void)      { return g_no_disc_wanted; }
int cd_iso_has_audio(void)           { return g_img != 0 || (g_tracks_total - g_tracks_missing) > 0; }
int cd_iso_tracks_missing(void)      { return g_tracks_missing; }
int cd_iso_tracks_total(void)        { return g_tracks_total; }
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
        ((struct ua32 *)((recs + i * 5 + 1)))->v = (uint32_t)lba;
    }
}

/* ---- raw sector read: the whole 2352 bytes, for CD-DA tracks ----
 * The LBAs the GAME passes here come from its own TOC table (fn_107b9 records): measured live on the
 * original at DAT_00053c64 -> track 2 starts at 37262, and fn_10b1d's playing window for it is
 * 37262..40415 (DAT_53c44/40, measured during the opening movie). Those are MSCDEX absolute LBAs
 * (LBA 0 = 2 s of disc lead-in). TG_NIGHT.IMG instead starts at the track-01 INDEX 01: sector 37111
 * is the last MODE1 sector (sync 00 FF FF FF FF FF FF 00) and 37112 the first audio sample of track 2
 * -- so disc LBA 37262 <-> file sector 37112: exactly -150, confirmed at the other end too
 * (measured lead-out 269453 - 150 = 269303 = the IMG's sector count). Reading the raw LBA as a file
 * index played every track 2 s late; that is the "wrong music" the 2026-10-26 playtest heard. */
int cd_iso_read_audio(uint32_t disc_lba, uint8_t *out2352) {
    if (!g_opened || !out2352) return -1;
    if (disc_lba < 150u) return -1;                      /* inside the 2 s of lead-in the disc never had */

    if (g_cd_dir[0] != '\0') {
        int i = cd_owner(disc_lba);
        if (i < 0) return -1;                            /* inside the data track: no file owns it */
        uint32_t off_sec = disc_lba - (uint32_t)g_toc[i].lba;
        if (off_sec >= g_sectors[i]) return -1;          /* past this track file's last frame */
        if (g_track_absent[i]) return -1;                /* no file: refused, not answered with silence */
        if (g_remote_pread) {
            /* One sector, exactly as the file path below reads it, and audited the same way: the
             * point of the audit is the byte range the game asked for, not how it was fetched. */
            char k[16]; snprintf(k, sizeof k, "TRACK%02u", g_toc[i].track);
            long long off = (long long)off_sec * IMG_SECTOR;
            if (g_remote_pread(g_toc[i].track, (uint64_t)off, out2352, IMG_SECTOR) != 0) return -1;
            cd_audit_read(k, off, IMG_SECTOR, out2352);
            return 0;
        }
        if (g_tf_track != g_toc[i].track) {
            char path[1200];
            if (g_tf) { fclose(g_tf); g_tf = 0; }
            snprintf(path, sizeof path, "%s/track%02u.bin", g_cd_dir, g_toc[i].track);
            g_tf = fopen(path, "rb");
            if (!g_tf) { g_tf_track = 0; g_tf_next = -1; return -1; }
            g_tf_track = g_toc[i].track; g_tf_next = -1;
        }
        long off = (long)off_sec * IMG_SECTOR;
        if (g_tf_next != off && fseek(g_tf, off, SEEK_SET) != 0) { g_tf_next = -1; return -1; }
        if (fread(out2352, 1, IMG_SECTOR, g_tf) != IMG_SECTOR) { g_tf_next = -1; return -1; }
        g_tf_next = off + IMG_SECTOR;
        /* kind names the track and the offset is relative to that track's file -- the same convention
         * pocket/src/night/of_disc.c records, so scripts/pocket/audit_disc.py maps both hosts onto the
         * shipped image with the same manifest rule (docs/410). */
        { char k[16]; snprintf(k, sizeof k, "TRACK%02u", g_toc[i].track);
          cd_audit_read(k, off, IMG_SECTOR, out2352); }
        return 0;
    }

    if (!g_img) return -1;          /* no byte source at all -- the compiled-in TOC has no samples */
    if (disc_lba - 150u >= g_total_sectors) return -1;
    if (fseek(g_img, (long)(disc_lba - 150u) * IMG_SECTOR, SEEK_SET) != 0) return -1;
    if (fread(out2352, 1, IMG_SECTOR, g_img) != IMG_SECTOR) return -1;
    cd_audit_read("IMG", (long long)(disc_lba - 150u) * IMG_SECTOR, IMG_SECTOR, out2352);
    return 0;
}

/* ---- logical sector read (MODE1/2352, user data at +16) ----
 * Only the single image answers these: the per-track layout carries audio tracks, and nothing in the
 * port reads ISO user data (the resources come out of NIGHT.RES), so a request in that mode is refused
 * rather than answered from a source that does not exist. */
int cd_iso_read_sector(uint32_t disc_lba, uint8_t *out2048) {
    if (!g_opened || !out2048 || !g_img) return -1;
    if (disc_lba >= g_total_sectors) return -1;
    if (fseek(g_img, (long)disc_lba * IMG_SECTOR + IMG_DATA_OFF, SEEK_SET) != 0) return -1;
    if (fread(out2048, 1, IMG_DATA_LEN, g_img) != IMG_DATA_LEN) return -1;
    cd_audit_read("IMG_USER", (long long)disc_lba * IMG_SECTOR + IMG_DATA_OFF, IMG_DATA_LEN, out2048);
    return 0;
}

int cd_iso_image_present(void) { return g_img != 0; }

void cd_iso_close(void) {
    cd_audit_close();
    if (g_img) fclose(g_img);
    if (g_tf) fclose(g_tf);
    g_img = 0; g_tf = 0; g_opened = 0; g_toc_n = 0;
    g_cd_dir[0] = '\0';
    g_no_disc = 0; g_require_disc = g_no_disc_wanted = 0;
    g_tracks_missing = g_tracks_total = 0;
}

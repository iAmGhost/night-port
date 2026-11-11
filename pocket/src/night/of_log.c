/* of_log.c -- diagnostics that survive having no reachable screen terminal (docs/335 §4).

 * The Pocket has no console, and the "Pocket LCD -> Terminal" entry never appeared on hardware, so the host
 * writes its own trail into bound nonvolatile data slots and the card is read back afterwards:
 *
 *   slot 8   night.cfg   (the SDK's shared-config slot: Assets/night/common/night.cfg)
 *
 * Slot 8 is the slot the SDK documents as writable from an app ("Slot 8 is reserved for SDK/shared config",
 * and the app save slots are "mapped to APF file IDs 10-19" -- README:466).  The first three hardware runs
 * used id 9, which is in neither category: fopen answered with a FILE*, every line was "written", and the
 * file on the card stayed at 0 bytes.  If slot 8 cannot be opened either, a save slot stands in (17, 16, 15).
 * Whichever file is used is named in the log itself.
 *
 * The SDK's documented order is register-then-fopen ("Apps register filenames at startup, then use standard C
 * file I/O", of_file_slot_register(id, name) then fopen(name)) with readdir("/") plus d_ino - 1 == slot id kept
 * as the fallback -- because a guessed path can wedge the slot bridge.  The scan is
 * also what proves the slot bridge at all: the entry list (name, inode, implied slot) goes into the log, so
 * a card read shows how the OS exposed the manifest to the app.  Every line is flushed as it is written, so
 * a core that is killed mid-run, faults, or never returns still left everything it knew on the card.
 */
#include "of_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef OF_PC
#include "of.h"
#include "of_file.h"
#include <dirent.h>
#endif

#ifndef SAVE_STAGE_FIRST_MIN
#define SAVE_STAGE_FIRST_MIN 10       /* README:644 reserves ids 10-19 for nonvolatile save .. night_6.sav */
#endif
#define LOG_SLOT        17         /* our own nonvolatile save slot.  README.md's data-slot table:
                                    * 10-19 are "Save 0-9", nonvolatile CRAM0, 256 KB each; slot 8 is
                                    * SDK-owned shared config.  0.2.23 deleted the seven game-save
                                    * bindings that used to sit in front of this one, and the card log
                                    * went silent with them (docs/451). */
#define LOG_NAME_0      "slot:17"  /* the explicit APF id form -- save_test.h:16: "the registered
                                    * filename, `save:N`, `save_N`, and `slot:10+N`".  FIRST, because an
                                    * id names a slot while `save:N` names a POSITION in the save list,
                                    * and 0.2.23 deleted seven positions.  save_test.h proves the four
                                    * paths reach the same save slot, and the write denial it tests is
                                    * for a read-only DATA slot (slot:2) -- not one in 10-19. */
#define LOG_NAME_2      "nightlog.sav"      /* the declared name.  README.md:468 calls this the PREFERRED
                                             * route -- "standard C file I/O with the save filename from
                                             * the instance JSON".  Second, not first: the registry is
                                             * populated by a directory scan, so a bare name has one more
                                             * thing to get right than an id does. */
#define LOG_NAME        "save:7"   /* the compat alias, slot 10+7.  THIRD: it is the name every log that
                                    * ever landed on hardware was opened on, so this is the fallback with
                                    * proof behind it. */
/* "log.sav" is OFF the chain on purpose.  The filename registry is populated by a directory scan
 * (README.md:654), so a name nothing declares is guesswork -- and guesswork on this filesystem is
 * silence, not an error.  Only names that bind to slot 17 belong in the chain (docs/451). */
#define LOG_FALLBACKS   { -1 }               /* no game save slot is ever a log target */

static FILE *g_log;
static char g_path[64];            /* the name the log is actually open on, for the reopen below */
static unsigned g_lines, g_commits;

/* SDK README:466 -- "Dirty save files are committed through the bridge when closed."  A FILE* held open for a
 * whole run is never closed, so under that rule nothing it wrote was ever eligible to reach the card: four
 * hardware runs flushed every line and night.log / night.cfg stayed at 0 bytes.  openfpgaOS-Quake never meets
 * the problem, because every quake.cfg and sN.sav write it does is fopen/fwrite/fclose.  So the log closes and
 * reopens on a cadence -- a commit a hung or killed run still reaches. */
static void log_commit(void) {
    if (!g_log || !g_path[0]) return;
    fclose(g_log);
    g_log = fopen(g_path, "ab");
    if (g_log) setvbuf(g_log, (char *)0, _IONBF, 0);
    g_commits++;
}

/* fopen plus the two things every open site needs: remember the name (for the reopen) and keep the line
 * buffer off, so a core that dies mid-run left what it knew. */
static FILE *log_open_write(const char *name) {
    FILE *f = fopen(name, "wb");            /* "wb", the mode openfpgaOS-Quake writes its cfg/saves with */
    if (!f) return 0;
    snprintf(g_path, sizeof g_path, "%s", name);
    setvbuf(f, (char *)0, _IONBF, 0);
    return f;
}

/* Every name tried and how it ended, whether or not the log ever opened. A whole class of failure used to
 * be invisible: four archives came back byte-identical (md5 6c1ee2d2..., 87 616 B -- the bytes a 0.2.21
 * boot wrote), and the only report of a failed open was a printf, which on hardware nobody can read.
 * "No new log" and "the log could not be opened" must be different observable states (docs/451). */
static char g_attempts[512];
static void attempt_add(const char *name, int ok) {
    if (strlen(g_attempts) + 96u >= sizeof g_attempts) return;
    snprintf(g_attempts + strlen(g_attempts), sizeof g_attempts - strlen(g_attempts),
             "  open \"%s\" -> %s\n", name, ok ? "WRITABLE" : "FAILED");
}
const char *of_log_attempts(void) { return g_attempts; }
long of_log_bytes(void) { return g_log ? ftell(g_log) : -1L; }   /* how much the OS has actually taken */

void of_log_close(void) {
    if (!g_log) return;
    fflush(g_log);
    fclose(g_log);
    g_log = 0;
    g_commits++;
}

#ifdef OF_PC
int of_log_open(void) {
    g_log = log_open_write("scratch/pocket/night.log");
    return g_log ? 0 : -1;
}
#else
/* Find the file a bound slot id resolves to, recording what the virtual root actually listed. */
static int slot_name(int want, char *out, size_t n, char *scan, size_t scan_n) {
    DIR *d = opendir("/");
    if (!d) { snprintf(scan + strlen(scan), scan_n - strlen(scan), "root: opendir(\"/\") failed\n"); return -1; }
    struct dirent *e;
    int found = 0, total = 0, shown = 0;
    while ((e = readdir(d)) != NULL) {
        total++;
        if (shown < 14) {
            shown++;
            snprintf(scan + strlen(scan), scan_n - strlen(scan), "root: \"%s\" ino=%ld slot=%d\n",
                     e->d_name, (long)e->d_ino, (int)e->d_ino - 1);
        }
        if ((int)e->d_ino - 1 == want) { snprintf(out, n, "%s", e->d_name); found = 1; }
    }
    closedir(d);
    snprintf(scan + strlen(scan), scan_n - strlen(scan), "root: %d entries; slot %d %s\n",
             total, want, found ? "found" : "absent");
    return found ? 0 : -1;
}

int of_log_open(void) {
    static const int want[] = LOG_FALLBACKS;
    static char scan[2048];
    char name[64];
    scan[0] = 0;

    /* No of_file_slot_register() any more, on purpose.  Quake -- the one core on this card whose writes are
     * known to land, Saves/quake/common/quake.cfg and s0.sav -- never calls it: sys_of.c:234 simply does
     * fopen("quake.cfg", "wb") and lets the kernel's boot-time name discovery stand, and of_file.h:16 says the
     * same ("Register a filename->data-slot binding ... Apps can still call this when they want to provide or
     * override a binding").  Registering a name ourselves claims that binding for the app, and the header
     * describes the registered result as "a read-only APF data file" -- which is a plausible reason a name we
     * registered took no writes across five hardware runs while Quake's unregistered name wrote 20 bytes and a
     * 35 KB save. */
    static const char *names[] = { LOG_NAME_0, LOG_NAME_2, LOG_NAME };   /* slot:17 (explicit id, immune to
                                                                          * ordinals), then the declared filename,
                                                                          * then the compat alias */
    g_attempts[0] = 0;
    for (unsigned i = 0; i < sizeof names / sizeof names[0]; i++) {
        if ((g_log = log_open_write(names[i]))) {
            attempt_add(names[i], 1);
            of_log_line("log: opened \"%s\" (%s)\n", names[i], i == 0 ? "slot:17, the APF id form" : "by name");
            if (slot_name(LOG_SLOT, name, sizeof name, scan, sizeof scan) == 0) of_log_line("%s", scan);
            else of_log_line("%s", scan);
            return 0;
        }
    }

    if (slot_name(LOG_SLOT, name, sizeof name, scan, sizeof scan) == 0 && (g_log = log_open_write(name))) {
        of_log_line("log: slot %d via \"%s\"\n", LOG_SLOT, name);
        of_log_line("%s", scan);
        return 0;
    }
    for (unsigned i = 0; i < sizeof want / sizeof want[0]; i++) {
        if (want[i] < (int)SAVE_STAGE_FIRST_MIN || want[i] > SAVE_STAGE_FIRST_MIN + 6) {
            printf("log: no writable slot; diagnostics stay on stdout only\n%s\n%s", scan, g_attempts);
            return -1;                        /* docs/393: a measuring boot may not eat a player's save slot */
        }
        scan[0] = 0;
        char bare[32];
        snprintf(bare, sizeof bare, "night_%d.sav", want[i] - 10);
        if ((g_log = log_open_write(bare))) {
            of_log_line("log: save slot %d used as the diagnostic trail via bare name \"%s\"\n", want[i], bare);
            slot_name(want[i], name, sizeof name, scan, sizeof scan);
            of_log_line("%s", scan);
            return 0;
        }
        if (slot_name(want[i], name, sizeof name, scan, sizeof scan) == 0 && (g_log = log_open_write(name))) {
            of_log_line("log: slot %d (save slot) used as the diagnostic trail via \"%s\"\n", want[i], name);
            of_log_line("%s", scan);
            return 0;
        }
    }
    printf("log: no writable slot; diagnostics stay on stdout only\n%s\n%s", scan, g_attempts);
    return -1;
}
#endif

void of_log_line(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fflush(g_log);
    if (++g_lines % 32u == 0u) log_commit();   /* was 8: ~2.4 SD writes/s on the card whose
                                                 * disc reads measured 23 ms mean /
                                                 * 122 ms max (docs/449 follow-up) */
}

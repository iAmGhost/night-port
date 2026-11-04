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

#define LOG_SLOT        8          /* SDK shared-config slot: the one an app is documented to write */
#define LOG_NAME        "night.cfg"
#define LOG_FALLBACKS   { 17, 16, 15 }       /* save slots: the nonvolatile type a core is known to write */

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
    g_log = fopen(g_path, "a");
    if (g_log) setvbuf(g_log, (char *)0, _IONBF, 0);
    g_commits++;
}

/* fopen plus the two things every open site needs: remember the name (for the reopen) and keep the line
 * buffer off, so a core that dies mid-run left what it knew. */
static FILE *log_open_write(const char *name) {
    FILE *f = fopen(name, "w");
    if (!f) return 0;
    snprintf(g_path, sizeof g_path, "%s", name);
    setvbuf(f, (char *)0, _IONBF, 0);
    return f;
}

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

    /* Register first, then fopen on the registered name -- the order README:440-448 documents, and the same
     * one openfpgaOS-Quake's sys_of.c uses for quake.cfg.  The d_ino scan is the fallback AND the witness:
     * whatever readdir did list gets written into the log. */
    of_file_slot_register(LOG_SLOT, LOG_NAME);
    if ((g_log = log_open_write(LOG_NAME))) {
        of_log_line("log: slot %d registered as \"%s\"\n", LOG_SLOT, LOG_NAME);
        if (slot_name(LOG_SLOT, name, sizeof name, scan, sizeof scan) == 0) of_log_line("%s", scan);
        else of_log_line("%s", scan);
        return 0;
    }

    if (slot_name(LOG_SLOT, name, sizeof name, scan, sizeof scan) == 0 && (g_log = log_open_write(name))) {
        of_log_line("log: slot %d via \"%s\"\n", LOG_SLOT, name);
        of_log_line("%s", scan);
        return 0;
    }
    for (unsigned i = 0; i < sizeof want / sizeof want[0]; i++) {
        scan[0] = 0;
        char bare[32];
        snprintf(bare, sizeof bare, "night_%d.sav", want[i] - 10);
        of_file_slot_register(want[i], bare);
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
    printf("log: no writable slot; diagnostics stay on stdout only\n%s", scan);
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
    if (++g_lines % 8u == 0u) log_commit();
}

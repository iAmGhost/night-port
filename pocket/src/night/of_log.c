/* of_log.c -- diagnostics that survive having no reachable screen terminal (docs/335 §4).

 * The Pocket has no console, and the "Pocket LCD -> Terminal" entry never appeared on hardware, so the host
 * writes its own trail into bound nonvolatile data slots and the card is read back afterwards:
 *
 *   slot 9   night.log   (Assets/night/common/night.log, 64 KiB maximum)
 *
 * If that slot cannot be opened, a save slot is used instead (17, 16, 15 in that order): save slots are the
 * one nonvolatile slot type openfpgaOS-Quake is known to write from an app, so the trail survives even when
 * this core's log slot is not writable.  Whichever file is used is named in the log itself.
 *
 * Slots are opened the way openfpgaOS-Quake's sys_of.c opens them -- readdir("/") plus d_ino - 1 == slot id,
 * then fopen on the name readdir reported -- because a guessed path can wedge the slot bridge.  The scan is
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
#include <dirent.h>
#endif

#define LOG_SLOT        9
#define LOG_FALLBACKS   { 17, 16, 15 }       /* save slots: the nonvolatile type a core is known to write */

static FILE *g_log;

#ifdef OF_PC
int of_log_open(void) {
    g_log = fopen("scratch/pocket/night.log", "w");
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

    if (slot_name(LOG_SLOT, name, sizeof name, scan, sizeof scan) == 0 && (g_log = fopen(name, "w"))) {
        setvbuf(g_log, (char *)0, _IONBF, 0);
        of_log_line("log: slot %d via \"%s\"\n", LOG_SLOT, name);
        of_log_line("%s", scan);
        return 0;
    }
    for (unsigned i = 0; i < sizeof want / sizeof want[0]; i++) {
        scan[0] = 0;
        if (slot_name(want[i], name, sizeof name, scan, sizeof scan) == 0 && (g_log = fopen(name, "w"))) {
            setvbuf(g_log, (char *)0, _IONBF, 0);
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
}

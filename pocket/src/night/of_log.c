/* of_log.c -- diagnostics that survive having no reachable screen terminal (docs/335 §4).

 * The Pocket has no console, and the "Pocket LCD -> Terminal" entry is an OS-side mode this core could not
 * reach on hardware, so the host writes its own trail into a bound nonvolatile data slot and the card is
 * read back afterwards:
 *
 *   slot 9   night.log   (Assets/night/common/night.log, 64 KiB maximum)
 *
 * The slot is opened the way openfpgaOS-Quake's sys_of.c opens its save slots -- readdir("/") plus
 * d_ino - 1 == slot id, then fopen on the name readdir reported -- because a guessed path can wedge the
 * slot bridge.  Every line is flushed as it is written, so a core that is killed mid-run, faults, or never
 * returns still left everything it knew on the card.
 */
#include "of_log.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef OF_PC
#include "of.h"
#include <dirent.h>
#endif

#define LOG_SLOT 9

static FILE *g_log;

int of_log_open(void) {
#ifdef OF_PC
    g_log = fopen("scratch/pocket/night.log", "w");
    return g_log ? 0 : -1;
#else
    DIR *d = opendir("/");
    if (!d) return -1;
    struct dirent *e;
    char name[64];
    int found = 0;
    while ((e = readdir(d)) != NULL) {
        if ((int)e->d_ino - 1 == LOG_SLOT) { snprintf(name, sizeof name, "%s", e->d_name); found = 1; break; }
    }
    closedir(d);
    if (!found) return -1;
    g_log = fopen(name, "w");                 /* truncate: a log describes the run it belongs to */
    if (g_log) setvbuf(g_log, (char *)0, _IONBF, 0);
    return g_log ? 0 : -1;
#endif
}

void of_log_line(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fflush(g_log);
}

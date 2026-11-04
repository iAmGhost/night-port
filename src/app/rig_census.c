/* Per-function entry census (docs/120) -- the coverage signal for input fuzzing.
 *
 * Built with `-finstrument-functions` (make censusapp): the compiler calls
 * __cyg_profile_func_enter for every entry into a function of this program, and the hook maps the address to
 * the generated table in src/gen/fn_table.c, whose names ARE the shipped guest addresses. The coverage
 * vector is therefore directly comparable to the Ghidra function set, which is what a fuzzer needs: "which
 * shipped functions has this input never reached".
 *
 * The table is emitted in name order (addresses are link-time), so a permutation is sorted once at startup
 * and binary-searched per call.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "gen/fn_table.h"

unsigned rig_census_report(const char *path);

static int  *g_order;                 /* index i -> g_fn_addr[g_order[i]] ascending */
static unsigned char *g_seen;
static int   g_ready;
static unsigned g_covered;

__attribute__((no_instrument_function))
static int cmp_addr(const void *a, const void *b)
{
    uintptr_t x = (uintptr_t)g_fn_addr[*(const int *)a];
    uintptr_t y = (uintptr_t)g_fn_addr[*(const int *)b];
    return (x > y) - (x < y);
}

static char g_path[600];

__attribute__((no_instrument_function))
static void on_term(int sig)
{
    (void)sig;
    rig_census_report(g_path[0] ? g_path : NULL);
    _exit(124);
}

__attribute__((no_instrument_function))
void rig_census_set_path(const char *path)
{
    if (path) { snprintf(g_path, sizeof g_path, "%s", path); }
}

__attribute__((no_instrument_function))
int rig_census_begin(void)
{
    if (g_ready) return 1;
    g_order = malloc(g_fn_count * sizeof *g_order);
    g_seen  = calloc(g_fn_count, 1);
    if (!g_order || !g_seen) return 0;
    for (unsigned i = 0; i < g_fn_count; i++) g_order[i] = (int)i;
    qsort(g_order, g_fn_count, sizeof *g_order, cmp_addr);
    signal(SIGTERM, on_term);      /* fuzzing kills on the wall-clock cap: keep the coverage anyway */
    g_ready = 1;
    return 1;
}

__attribute__((no_instrument_function))
void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
    (void)call_site;
    if (!g_ready) return;
    int lo = 0, hi = (int)g_fn_count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        uintptr_t v = (uintptr_t)g_fn_addr[g_order[mid]];
        if (v == (uintptr_t)this_fn) {
            int idx = g_order[mid];
            if (!g_seen[idx]) { g_seen[idx] = 1; g_covered++; }
            return;
        }
        if (v < (uintptr_t)this_fn) lo = mid + 1; else hi = mid - 1;
    }
}

__attribute__((no_instrument_function))
void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    (void)this_fn; (void)call_site;
}

/* Write the census: one line per function, "<seen|never> <name>", plus a compact bitmap the fuzz driver can
 * union without re-reading names. Returns the number entered. */
__attribute__((no_instrument_function))
unsigned rig_census_report(const char *path)
{
    if (!g_ready) return 0;
    if (path && !g_path[0]) snprintf(g_path, sizeof g_path, "%s", path);
    FILE *f = path ? fopen(path, "w") : stderr;
    if (!f) return g_covered;
    fprintf(f, "# function entry census: %u/%u entered\n", g_covered, g_fn_count);
    for (unsigned i = 0; i < g_fn_count; i++)
        fprintf(f, "%s %s\n", g_seen[i] ? "seen" : "never", g_fn_name[i]);
    if (f != stderr) fclose(f);
    char bp[1100];
    if (path) snprintf(bp, sizeof bp, "%s", path); else snprintf(bp, sizeof bp, "census.txt");
    char *dot = strrchr(bp, '.');
    if (dot && !strcmp(dot, ".txt")) *dot = 0;
    strncat(bp, ".bin", sizeof bp - strlen(bp) - 1);
    FILE *b = fopen(bp, "wb");
    if (b) { fwrite(g_seen, 1, g_fn_count, b); fclose(b); }
    fprintf(stderr, "census: %u/%u shipped-named functions entered (%s)\n", g_covered, g_fn_count,
            path ? path : "-");
    return g_covered;
}

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
#include <stdlib.h>          /* malloc: the LOG=3 ring lives in SDRAM (docs/456 13) */

static int g_ram;                  /* LOG=3 ring on/off -- of_log_close() sits above the ring code */
static int g_null;                 /* LOG=4: format and discard -- no open, no handle, no bytes (docs/456 13) */
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

static void ram_flush(void);

void of_log_close(void) {
    if (g_null) return;                          /* nothing open, nothing to commit */
    if (g_ram) ram_flush();          /* commits and closes; atexit(of_log_close) is the quit path */
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
    /* LOG=4: refuse to open anything, and say so. Returning 0 keeps g_log_ok set, because the readout at
     * main_of.c:3225 uses g_log_ok to decide whether the in-game console takes over -- clearing it here
     * would add a second variable to the boot this arm exists to isolate. */
    if (g_null) {
        snprintf(g_attempts, sizeof g_attempts,
                 "  NULL SINK: no slot opened, no handle held, nothing written (LOG=4)\n");
        printf("log: NULL SINK -- every line formatted and discarded; the card sees nothing at all\n");
        return 0;
    }
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

/* LOG=2: the run's shape, not its minutes.
 *
 * WHY THIS EXISTS.  docs/456 13 settled it with the bits held still: 0.3.6 booted LOG=1 three times is
 * called smooth, booted LOG=0 is called stuttering, and the card's exFAT verified clean -- so the log is
 * not merely an observer here, it is (somehow) the anesthetic.  That leaves the shipped player shape in
 * the worst of both worlds: the configuration we do NOT want, which also cannot testify to what it did.
 * Summary mode keeps a player boot evidential at a fraction of the SD traffic -- one compact set per
 * report (~6 lines: settings/blit shape at boot, then perf, split wall, gap worst, stages last, view=)
 * instead of every line the app can produce.
 *
 * The filter is a prefix table in ONE place.  Classifying ~200 call sites is how a logging change turns
 * into a half-rewrite, and a line that fails the table is still formatted, so a summary log's lines are
 * byte-identical to the same lines in a full log -- branchfps.py reads either. */
static int g_summary;

void of_log_summary(int on) { g_summary = on; }

/* ---- LOG=3: the ring ----
 *
 * WHY.  docs/456 13 left two live explanations for the same measurement -- "the report block's write
 * events change how the next big read behaves" and "the report block's own work re-spaces the loop" --
 * and neither can be tested with LOG=1/LOG=2, because in both of those the SD events and the CPU work
 * arrive together.  RAM mode splits them: the report block formats every line into a ring in SDRAM and
 * the card is touched once per LOG_FLUSH_MS by one stream append.  If the stutter comes back, the events
 * were the thing; if it stays away, it was never about the card at all -- and we can say which.
 *
 * The ring is tail-preserving: when it overflows the OLDEST bytes are lost and `lost` counts them, so a
 * dump always describes the most recent ~30 s rather than the oldest minute.  Nothing is written per
 * line: the FILE* is opened once (so path and slot resolution, and the "log: opened" line, still work)
 * and only of_log_flush() pushes bytes to it, with the periodic commit deliberately NOT run. */
#define LOG_RAM_BYTES (256u * 1024u)

static int      g_ram_period_ms;
static char    *g_rb;
static size_t   g_rb_cap, g_rb_head, g_rb_len, g_rb_since;
static unsigned g_ram_lost, g_ram_flushes, g_ram_next_ms;

void of_log_ram_enable(int on, unsigned period_ms) {
    g_ram = on;
    g_ram_period_ms = period_ms > 1000u ? period_ms : 30000u;
}

int of_log_ram_enabled(void) { return g_ram; }

void of_log_null(int on) { g_null = on; }
int  of_log_null_enabled(void) { return g_null; }

void of_log_ram_stats(unsigned *buffered, unsigned *lost, unsigned *flushes) {
    if (buffered) *buffered = (unsigned)g_rb_len;
    if (lost) *lost = g_ram_lost;
    if (flushes) *flushes = g_ram_flushes;
}

/* Push everything buffered since the last flush, in order, unwrapping the ring -- and CLOSE the file
 * doing it.  The close is not tidiness: README:466 ("Dirty save files are committed through the bridge
 * when closed") is the rule that made the first four hardware runs flush every line and land nothing, and
 * the same rule is why log_commit() exists at 32-line intervals.  An append to a FILE* that is held open
 * for a whole run is an append that never happened.  The first LOG=3 boot read void for exactly this
 * reason -- no bytes on the card, the previous LOG=2 boot's log byte-identical under it (docs/456 13).
 *
 * So each flush is open("ab") / write / close, which is also what Quake does for its cfg and saves
 * (sys_of.c:234).  The cost is one open+close pair per LOG_FLUSH_MS, i.e. still ~1 card event every 30 s
 * instead of ~6 per second, and the gain is that the record exists no matter how the boot ends. */
static void ram_flush(void) {
    if (!g_rb || g_rb_since == 0u || !g_path[0]) return;
    if (g_log) { fclose(g_log); g_log = 0; }
    g_log = fopen(g_path, "ab");
    if (!g_log) return;
    setvbuf(g_log, (char *)0, _IONBF, 0);
    size_t n = g_rb_since > g_rb_len ? g_rb_len : g_rb_since;
    size_t start = (g_rb_head + g_rb_cap - n) % g_rb_cap;
    size_t first = g_rb_cap - start;
    if (first > n) first = n;
    fwrite(g_rb + start, 1, first, g_log);
    if (n > first) fwrite(g_rb, 1, n - first, g_log);
    fflush(g_log);
    fclose(g_log); g_log = 0;            /* the commit -- do not "leave it open for speed" */
    g_ram_flushes++;
    g_rb_since = 0u;
}

void of_log_tick(uint64_t now_us) {
    if (!g_ram) return;
    unsigned now_ms = (unsigned)(now_us / 1000ull);
    if ((int)(now_ms - g_ram_next_ms) >= 0) {
        ram_flush();
        g_ram_next_ms = now_ms + (unsigned)g_ram_period_ms;
    }
}

static const char *const SUMMARY[] = {
    "core ",            /* the boot marker: without it a log cannot be attributed to a build (docs/428) */
    "log: ",            /* which slot the trail landed in, and the scan that found it */
    "settings:",        /* the shape this boot ran in -- the whole point of a player-shape log */
    "blit shape:", "fit=", "fb mode", "view=",
    "perf ", "split wall", "gap worst", "stages last", "STALL", "video CHANGED",
    "ramlog",            /* LOG=3: proof the ring is filling and how rarely the card is touched */
    "ping:",             /* what the read-ping arm is set to -- a shape line, and it has confused a boot */
    "audio ring", "cdda ", "clock:", "memstat", NULL,
};

static int summary_worthy(const char *s) {
    for (int i = 0; SUMMARY[i]; i++)
        if (strncmp(s, SUMMARY[i], strlen(SUMMARY[i])) == 0) return 1;
    return 0;
}

void of_log_line(const char *fmt, ...) {
    if (!g_log && !g_ram && !g_null) return;   /* RAM mode needs no open FILE: the ring is the sink until it flushes */
    static char buf[4096];                       /* the longest line today is the slot scan at 2048 */
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (g_summary && !summary_worthy(buf)) return;
    if (g_null) return;                          /* LOG=4: the work was done, the result is thrown away */
    if (g_ram) {                                    /* LOG=3: SDRAM ring, no SD per line (docs/456 13) */
        if (!g_rb) { g_rb = (char *)malloc(LOG_RAM_BYTES); g_rb_cap = g_rb ? LOG_RAM_BYTES : 0u; }
        if (g_rb_cap == 0u) return;                 /* no ring: the line is simply gone */
        size_t ln = (size_t)(n < 0 ? 0 : n);
        if (ln > sizeof buf - 1u) ln = sizeof buf - 1u;
        if (ln > g_rb_cap) {                        /* never taken at 4 KB lines into a 256 KB ring, */
            memmove(buf, buf + (ln - g_rb_cap), g_rb_cap);   /* but keep the TAIL if it ever is */
            ln = g_rb_cap;
        }
        size_t head = ln;                           /* bytes to place */
        size_t first = g_rb_cap - g_rb_head;
        if (first > head) first = head;
        memcpy(g_rb + g_rb_head, buf, first);
        if (head > first) memcpy(g_rb, buf + first, head - first);
        g_rb_head = (g_rb_head + head) % g_rb_cap;
        g_rb_len += head; g_rb_since += head;
        if (g_rb_len > g_rb_cap) {
            g_ram_lost += (unsigned)(g_rb_len - g_rb_cap);
            g_rb_len = g_rb_cap;
        }
        return;
    }
    fputs(buf, g_log);
    fflush(g_log);
    if (++g_lines % 32u == 0u) log_commit();   /* was 8: ~2.4 SD writes/s on the card whose
                                                 * disc reads measured 23 ms mean /
                                                 * 122 ms max (docs/449 follow-up) */
}

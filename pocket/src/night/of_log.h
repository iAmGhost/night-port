/* of_log.h -- the on-card diagnostic log: data slot 8 (night.cfg), readable without a terminal. */
#ifndef NIGHT_OF_LOG_H
#define NIGHT_OF_LOG_H

#include <stdint.h>              /* uint64_t for of_log_tick() */

/* Open the log slot (id 8, "night.cfg").  0 on success, -1 when the slot is not there.  The desktop build
 * writes scratch/pocket/night.log so a PC run leaves the same witness. */
int  of_log_open(void);

/* One line: written to stdout AND appended to the log, flushed per line.  A core that is killed, faults,
 * or simply never returns leaves its whole trail on the card. */
void of_log_line(const char *fmt, ...);
/* LOG=2 (summary-only): keep the run's shape, drop its minutes.  Set once after of_log_open(); a line
 * whose first token is not on the summary table is formatted and discarded.  docs/456 13. */
void of_log_summary(int on);

/* LOG=3 (RAM): every line goes to an SDRAM ring, and the ring is APPENDED to the card once every
 * period_ms -- so the report block still runs (its CPU work happens either way) while the card sees
 * almost nothing.  This is the arm that separates the two surviving explanations of the anesthetic:
 * the periodic per-line SD event, or the report block's own work re-spacing the loop (docs/456 13).
 * Call of_log_tick() from the once-a-second report to run the flush schedule. */
void of_log_ram_enable(int on, unsigned period_ms);

/* LOG=4 (NULL SINK): the report block formats every line exactly as at LOG=1 and the line is DISCARDED.
 * No file is opened, no slot is scanned, no handle is held -- so the two surviving explanations of the
 * anesthetic separate cleanly (docs/456 13): if this boot is smooth the anesthetic is the WORK (our code,
 * our clock); if it stutters, the anesthetic was the OPEN HANDLE, and the mechanism is in the OS's
 * save-slot handling rather than in anything we write. */
void of_log_null(int on);
int  of_log_null_enabled(void);
int  of_log_ram_enabled(void);
void of_log_tick(uint64_t now_us);
void of_log_ram_stats(unsigned *buffered, unsigned *lost, unsigned *flushes);

void of_log_close(void);
long of_log_bytes(void);                 /* ftell of the log, or -1 when nothing is open */                /* fclose, which is what commits a nonvolatile slot */

#endif /* NIGHT_OF_LOG_H */

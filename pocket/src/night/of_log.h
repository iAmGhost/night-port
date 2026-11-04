/* of_log.h -- the on-card diagnostic log: data slot 9 (night.log), readable without a terminal. */
#ifndef NIGHT_OF_LOG_H
#define NIGHT_OF_LOG_H

/* Open the log slot (id 9, "night.log").  0 on success, -1 when the slot is not there.  The desktop build
 * writes scratch/pocket/night.log so a PC run leaves the same witness. */
int  of_log_open(void);

/* One line: written to stdout AND appended to the log, flushed per line.  A core that is killed, faults,
 * or simply never returns leaves its whole trail on the card. */
void of_log_line(const char *fmt, ...);

#endif /* NIGHT_OF_LOG_H */

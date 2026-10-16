/* sys.h — modern replacements for DOS/OS services the original functions used.
 * f/ code is 1:1 with the original; sys/ is where the modern world plugs in. */
#ifndef NIGHT_SYS_H
#define NIGHT_SYS_H

#include <stdint.h>

/* x86 I/O ports (input board on 0x3C7-0x3C9). Routed through a virtual
 * port map so tests can drive them; a real backend can replace this. */
void sys_outb(uint16_t port, uint8_t v);
uint8_t sys_inb(uint16_t port);
void sys_ports_reset(void);
void sys_port_set(uint16_t port, uint8_t v);   /* seed for tests (inb side) */
int  sys_hist(void *buf, int max);              /* 0x3C9 write history (tests) */

/* SS segment: original canary compared the SS register; the modern flat
 * model has a single address space, so SS is a constant. */
uint16_t sys_ss(void);

/* Fatal path stand-in for the original 0x429ea ("Stack Overflow!" printer)
 * and the fall-through into the next function (process death). The hook
 * lets tests observe the fatal path instead of aborting. */
void sys_fatal(const char *msg);
void sys_set_fatal_handler(void (*fn)(const char *msg));  /* NULL = default abort */

#endif

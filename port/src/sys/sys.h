/* sys.h — modern replacements for DOS/OS services the original functions used.
 * f/ code is 1:1 with the original; sys/ is where the modern world plugs in. */
#ifndef NIGHT_SYS_H
#define NIGHT_SYS_H

#include <stdint.h>

/* x86 I/O ports (0x0-0x3ff: 8259 PIC, 8253 PIT, DIP board 0x3C7-0x3C9, CD
 * audio). Routed through a virtual port map so tests can drive them; a real
 * backend can replace this. */
void sys_outb(uint16_t port, uint8_t v);
uint8_t sys_inb(uint16_t port);
void sys_ports_reset(void);
void sys_port_set(uint16_t port, uint8_t v);   /* seed for tests (inb side) */
int  sys_hist(void *buf, int max);              /* 0x3C9 write history (tests) */

/* Interrupt-vector service (INT 0x21h get/set). No real vectors in a flat
 * process; the timer subsystem builds a tick driver on sys_isr_set. */
uint64_t sys_isr_get(uint8_t vector);
void     sys_isr_set(uint8_t vector, uint32_t handler_off, uint16_t handler_seg);

/* SS segment: original canary compared the SS register; the modern flat
 * model has a single address space, so SS is a constant. */
uint16_t sys_ss(void);

/* Segment snapshot for DPMI real-mode transition bookkeeping (the original
 * reads the six x86 segment registers). The modern flat model has no
 * meaningful per-register values, so this returns a fixed table in the order
 * ES, CS, SS, DS, FS, GS. */
void sys_get_segments(uint16_t out[6]);

/* Fatal path stand-in for the original 0x429ea ("Stack Overflow!" printer)
 * and the fall-through into the next function (process death). The hook
 * lets tests observe the fatal path instead of aborting. */
void sys_fatal(const char *msg);
void sys_set_fatal_handler(void (*fn)(const char *msg));  /* NULL = default abort */

/* Key input (DOS INT 21h AH=08h, direct character input). Returns the key byte
 * (DOS ASCII/scan), or 0 if no key is available. Stub for now; wired to the
 * SDL keyboard when the input subsystem is ported. */
uint8_t sys_getkey(void);
/* Key input, cooked/echo (DOS INT 21h AH=01h). Stub like sys_getkey. */
uint8_t sys_getkey_echo(void);

/* Breakpoint hook for the original INT3 debug gate (0x45a44). No-op unless a
 * debugger is attached (the original INT3 stops under a debugger). */
void sys_breakpoint(uint32_t eax, uint16_t edx);

/* DOS clock (INT 21h/0x2Ch) stand-ins. sys_clock_ticks_per_sec returns the
 * tick rate (default 18, the DOS 18.2 Hz rate); tests may override it.
 * sys_clock_read performs one clock read (a counting stub). */
extern uint32_t sys_clock_tps;
uint32_t sys_clock_ticks_per_sec(void);
extern uint32_t sys_clock_read_count;
void sys_clock_read(void);

/* DOS IOCTL (INT 21h/0x44h/AL=0) stand-in. Returns a controllable (dx, cf) for
 * tests; default 0. The real device backend is not wired yet. */
extern uint32_t sys_ioctl_dx, sys_ioctl_cf;
void sys_dos_ioctl(uint32_t handle, uint32_t *out_dx, uint32_t *out_cf);

/* DOS PM/real-mode disk+segment setup (INT 2Fh / INT 21h/0x30h). No-op in the
 * modern port (the OS provides disk/segment management). */
void sys_dos_realmode_setup(void);
/* Standard x87 FPU control word (round-to-nearest, extended precision, all
 * exceptions masked) = 0x037F. */
uint16_t sys_fpu_control_word(void);

/* DOS date/time (INT 21h/0x2Ah get time, INT 21h/0x2Ch get date). Controllable
 * via extern globals so tests can drive fixed values; a real backend can fill
 * these from the host clock. */
struct dos_date { uint8_t year, month, day, dow; };
struct dos_time { uint8_t hour, minute, second, centisec; };
extern struct dos_date sys_date;
extern struct dos_time sys_time;
struct dos_date sys_getdate(void);
struct dos_time sys_gettime(void);

/* DOS INT 21h file operations (handle-based). No CD/file backend is wired yet,
 * so each returns a documented no-op default; the real TG_NIGHT.IMG/.CUE backend
 * will override these in the mid-subsystems task. */
int32_t sys_file_read(uint32_t handle, void *buf, uint32_t count);      /* AH=3f: bytes read, default 0 */
int32_t sys_file_lseek(uint32_t handle, int32_t offset, int32_t whence); /* AH=42: new pos, default 0 */
int32_t sys_file_write(uint32_t handle, const void *buf, uint32_t count); /* AH=40: bytes written; no backend -> success (count) */
int32_t sys_file_close(uint32_t handle);                                /* AH=3e: 0=success (CF clear), nonzero=error */
int32_t sys_file_open(const char *name, uint32_t mode);                 /* AH=3d: handle>=0 success, <0 error; no backend -> sys_open_ret */
int32_t sys_file_create(const char *name, uint32_t mode);               /* AH=3c: handle>=0 success, <0 error; no backend -> sys_create_ret */
int32_t sys_file_delete(const char *name);                             /* AH=41: 0 on success, default 0 */
extern int32_t sys_open_ret;    /* test knob: sys_file_open return (default 0=success, handle 0) */
extern int32_t sys_file_lseek_pos; /* test knob: position reported by a tell (whence==1); default 0 */
extern int32_t sys_create_ret;  /* test knob: sys_file_create return (default 0=success, handle 0) */
void sys_file_open_queue(const int32_t *vals, int n);  /* test: enqueue N open results, then fall back to sys_open_ret */
void sys_file_open_reset(void);                          /* test: clear the queued open results */

#endif

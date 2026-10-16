/* sys.c — modern backend for DOS services (see sys.h). */
#include "sys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- virtual port map (low I/O ports 0x0-0x3ff) ----
 * Covers the ports the game actually touches: 8259 PIC (0x20-0x21), 8253 PIT
 * (0x40-0x43), the DIP input board (0x3C7-0x3C9), and CD-audio/DMA ranges.
 * Routed through a virtual array so tests can replay the I/O the original
 * hardware would have seen; a real backend can replace this. */
#define PORT_MAX 0x400
static uint8_t port_val[PORT_MAX];
/* write history for 0x3C9 (board data port) — lets tests replay the
 * OUTSB stream the original hardware would have seen */
#define HIST_MAX 256
static uint8_t hist[HIST_MAX];
static int hist_n;

void sys_ports_reset(void) {
    memset(port_val, 0, sizeof port_val);
    hist_n = 0;
}

int sys_hist(void *buf, int max) {            /* returns history length */
    if (hist_n > max) hist_n = max;
    memcpy(buf, hist, (size_t)hist_n);
    return hist_n;
}

void sys_outb(uint16_t port, uint8_t v) {
    if (port < PORT_MAX) {
        port_val[port] = v;
        if (port == 0x3c9 && hist_n < HIST_MAX)
            hist[hist_n++] = v;
    }
    /* other ports: no-op (no real hardware in the port) */
}

uint8_t sys_inb(uint16_t port) {
    if (port < PORT_MAX)
        return port_val[port];
    return 0;
}

void sys_port_set(uint16_t port, uint8_t v) {
    if (port < PORT_MAX)
        port_val[port] = v;
}

/* ---- interrupt-vector service (INT 0x21h get/set) ----
 * The original hooks the PIT via INT 0x1C and programs the 8259/8253
 * directly. A flat SDL process has no real interrupt vectors, so these are
 * stubs; the timer subsystem (batch: 0x0003ee88/0x0003ef29) will build a
 * tick driver on top of sys_isr_set. */
uint64_t sys_isr_get(uint8_t vector) {
    (void)vector;
    return 0;                 /* no real OS vector to read */
}

void sys_isr_set(uint8_t vector, uint32_t handler_off, uint16_t handler_seg) {
    (void)vector; (void)handler_off; (void)handler_seg;
    /* no-op now: installing a real tick handler is the timer subsystem's job */
}

/* ---- SS / fatal ---- */
uint16_t sys_ss(void) { return 0x2b; } /* flat model: constant segment */

void sys_get_segments(uint16_t out[6]) {
    /* order: ES, CS, SS, DS, FS, GS (matches the original's read order).
     * Flat long-mode model: data segments are 0, CS/SS are the canonical
     * selectors; FS/GS base regs are 0 in this process. */
    out[0] = 0x0000; /* ES */
    out[1] = 0x0023; /* CS */
    out[2] = 0x002b; /* SS (== sys_ss) */
    out[3] = 0x0000; /* DS */
    out[4] = 0x0000; /* FS */
    out[5] = 0x0000; /* GS */
}

static void (*fatal_hook)(const char *) = 0;

void sys_set_fatal_handler(void (*fn)(const char *msg)) { fatal_hook = fn; }

void sys_fatal(const char *msg) {
    if (fatal_hook) {
        fatal_hook(msg);
        return;
    }
    fputs(msg, stderr);
    fflush(stderr);
    abort();
}

/* Key input (DOS INT 21h AH=08h). Stub: the real keyboard backend is wired in
 * when the input subsystem is ported. Returns 0 (no key) for now. */
uint8_t sys_getkey(void) { return 0; }

/* Key input, cooked/echo (DOS INT 21h AH=01h). Stub, like sys_getkey. */
uint8_t sys_getkey_echo(void) { return 0; }

/* Breakpoint hook for the INT3 debug gate. No-op (the original INT3 only stops
 * under an attached debugger). Kept as a hook so a debugger could attach. */
void sys_breakpoint(uint32_t eax, uint16_t edx) { (void)eax; (void)edx; }

uint32_t sys_clock_tps = 18;   /* DOS 18.2 Hz tick rate (default) */
uint32_t sys_clock_ticks_per_sec(void) { return sys_clock_tps; }
uint32_t sys_clock_read_count = 0;
void sys_clock_read(void) { sys_clock_read_count++; }

uint32_t sys_ioctl_dx = 0, sys_ioctl_cf = 0;
void sys_dos_ioctl(uint32_t handle, uint32_t *out_dx, uint32_t *out_cf) {
    (void)handle; *out_dx = sys_ioctl_dx; *out_cf = sys_ioctl_cf;
}
void sys_dos_realmode_setup(void) { /* OS handles disk/segment management */ }
uint16_t sys_fpu_control_word(void) { return 0x037f; }

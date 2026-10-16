/* sys.c — modern backend for DOS services (see sys.h). */
#include "sys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- virtual port map (0x3C0-0x3CF, LPT1/DIP board region) ---- */
static uint8_t port_val[0x10];
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
    if (port >= 0x3c0 && port < 0x3d0) {
        port_val[port - 0x3c0] = v;
        if (port == 0x3c9 && hist_n < HIST_MAX)
            hist[hist_n++] = v;
    }
    /* other ports: no-op (no real hardware in the port) */
}

uint8_t sys_inb(uint16_t port) {
    if (port >= 0x3c0 && port < 0x3d0)
        return port_val[port - 0x3c0];
    return 0;
}

void sys_port_set(uint16_t port, uint8_t v) {
    if (port >= 0x3c0 && port < 0x3d0)
        port_val[port - 0x3c0] = v;
}

/* ---- SS / fatal ---- */
uint16_t sys_ss(void) { return 0x2b; } /* flat model: constant segment */

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

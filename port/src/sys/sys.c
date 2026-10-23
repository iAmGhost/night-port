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
#define HIST_MAX 0x40000   /* grown: a whole interactive screen (fade-in + page loads + fade-out, ~80 KB of 0x3C9 bytes) must fit without saturating */
/* Heap-allocated on purpose: a 256 KB BSS array shifts the image layout and collides with the
 * fixed-address arena mapping the real-data tests use (SIGBUS inside the arena). */
static uint8_t *hist;
static int hist_n;

static uint8_t *hist_buf(void) {
    if (!hist) hist = (uint8_t *)calloc(1, HIST_MAX);
    return hist;
}

/* ---- PIT: mode register 0x43 selects channel/lo-hi/mode; 0x40/0x41/0x42 carry the divisor ---- */
static uint32_t pit_div[3];      /* programmed divisor per channel (0 = never programmed) */
static uint8_t  pit_mode[3];     /* the mode byte last selected for that channel */
static uint8_t  pit_phase[3];    /* 0 = low byte next, 1 = high byte next */

uint32_t sys_pit_divisor(int ch) { return (ch >= 0 && ch < 3) ? pit_div[ch] : 0; }
uint32_t sys_pit_hz(int ch) {
    uint32_t d = sys_pit_divisor(ch);
    return d ? (1193182u / d) : 0u;
}

static void pit_out(uint16_t port, uint8_t v) {
    if (port == 0x43) {
        int ch = (v >> 6) & 3;                    /* bits 7-6: channel (3 = read-back command) */
        if (ch < 3) {
            pit_mode[ch] = v;
            pit_phase[ch] = ((v & 0x30) == 0x10) ? 1 : 0;   /* 01 = write low only, 11 = low then high */
            if ((v & 0x30) == 0x30) pit_div[ch] = 0;         /* a new 16-bit program starts fresh */
        }
        return;
    }
    int ch = port - 0x40;
    if (ch < 0 || ch > 2) return;
    if (pit_phase[ch] == 0) { pit_div[ch] = (pit_div[ch] & 0xff00u) | v; pit_phase[ch] = 1; }
    else                    { pit_div[ch] = (pit_div[ch] & 0x00ffu) | ((uint32_t)v << 8); pit_phase[ch] = 0; }
}

void sys_ports_reset(void) {
    memset(port_val, 0, sizeof port_val);
    hist_n = 0;
    sys_vga_palette_reset();
    memset(pit_div, 0, sizeof pit_div);
    memset(pit_mode, 0, sizeof pit_mode);
    memset(pit_phase, 0, sizeof pit_phase);
}

int sys_hist(void *buf, int max) {            /* returns history length */
    uint8_t *hb = hist_buf();
    if (hist_n > max) hist_n = max;
    if (hb) memcpy(buf, hb, (size_t)hist_n);
    return hist_n;
}

/* ---- VGA DAC state ---- */
static uint8_t  dac_rgb[768];
static uint8_t dac_max;
static uint32_t dac_writes;
static uint8_t  dac_index;      /* the 0x3C8 write index */
static uint8_t  dac_phase;      /* 0=R, 1=G, 2=B within the current entry */

/* A single palette register programmed directly (VBE INT 10h AX=4F09 style), as opposed to the
 * legacy 0x3C8/0x3C9 stream. Both end up in the same DAC table. */
void sys_vga_palette_set(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= 256) return;
    dac_rgb[index * 3u] = r; dac_rgb[index * 3u + 1] = g; dac_rgb[index * 3u + 2] = b;
    uint8_t mx = r > g ? r : g; if (b > mx) mx = b;
    if (mx > dac_max) dac_max = mx;
    dac_writes += 3;
}

void sys_vga_palette(uint8_t *out768) { if (out768) memcpy(out768, dac_rgb, 768); }
uint8_t sys_vga_dac_max(void) { return dac_max; }

uint32_t sys_vga_palette_writes(void) { return dac_writes; }
void sys_vga_palette_reset(void) { memset(dac_rgb, 0, sizeof dac_rgb); dac_writes = 0; dac_index = 0; dac_phase = 0; }

/* ---- input pump hook ---- */
static sys_pump_fn input_pump;
void sys_set_input_pump(sys_pump_fn fn) { input_pump = fn; }
sys_pump_fn sys_get_input_pump(void) { return input_pump; }

static sys_pump_fn tick_pump;
void sys_set_tick_pump(sys_pump_fn fn) { tick_pump = fn; }
sys_pump_fn sys_get_tick_pump(void) { return tick_pump; }

/* ---- device port model: separate read (status) and write (command) sides ---- */
#define DEV_PORT_BASE 0x200          /* the ISA device range the base scan probes: 0x200..0x2ff */
static uint8_t     dev_status[PORT_MAX];
static uint8_t     dev_status_set[PORT_MAX];
static uint32_t    dev_cmds[PORT_MAX];
static uint8_t     dev_last[PORT_MAX];
static sys_dev_cmd_fn dev_hook;
static const uint8_t *dev_seq[PORT_MAX];
static int            dev_seq_len[PORT_MAX];
static int            dev_seq_at[PORT_MAX];

void    sys_dev_status_set(uint16_t port, uint8_t v) { if (port < PORT_MAX) { dev_status[port] = v; dev_status_set[port] = 1; } }
uint8_t sys_dev_status(uint16_t port) { return port < PORT_MAX ? dev_status[port] : 0; }
void    sys_dev_status_reset(void) { memset(dev_status, 0, sizeof dev_status); memset(dev_status_set, 0, sizeof dev_status_set);
                                     memset(dev_cmds, 0, sizeof dev_cmds); memset(dev_last, 0, sizeof dev_last);
                                     memset(dev_seq, 0, sizeof dev_seq); memset(dev_seq_len, 0, sizeof dev_seq_len);
                                     memset(dev_seq_at, 0, sizeof dev_seq_at);
                                     dev_hook = 0; }
void sys_dev_read_seq(uint16_t port, const uint8_t *bytes, int n) {
    if (port >= PORT_MAX) return;
    dev_seq[port] = bytes; dev_seq_len[port] = bytes ? n : 0; dev_seq_at[port] = 0;
    if (bytes && n > 0) dev_status_set[port] = 1;
}

void    sys_dev_cmd_hook(sys_dev_cmd_fn fn) { dev_hook = fn; }
uint32_t sys_dev_cmds(uint16_t port)  { return port < PORT_MAX ? dev_cmds[port] : 0; }
uint8_t  sys_dev_last_cmd(uint16_t port) { return port < PORT_MAX ? dev_last[port] : 0; }

/* ---- Sound Blaster / 8237 audio path ----------------------------------------------------------
 * The game drives its sound effects exactly the way a Sound Blaster is driven: DSP commands on the
 * board command port (base+0xc), and the sample bytes handed over through an ISA DMA transfer that it
 * programs itself (fn_040b83: mask, clear ff, preset 0x49, address on 0x01, page on 0x83, count on
 * 0x03, unmask). Command 0x40 + time constant sets the sample rate (rate = 1e6 / (256 - t)); 0x14 /
 * 0x17 start an 8-bit DMA transfer, 0x75 / 0x77 a 16-bit one, 0xd0 / 0xd1 stop and reset.
 * The model below recognises that protocol on sys_outb and turns each transfer into PCM samples in a
 * ring buffer, which the SDL host drains. PORT DEVIATION: the DMA source address in this port is a
 * truncated host pointer, so the host registers the buffer the transfer reads (sys_audio_set_source);
 * the original reads physical memory at the DMA address. */
#define AUDIO_RING (1u << 17)
static uint8_t  au_ring[AUDIO_RING];
static uint32_t au_w, au_r;
static uint32_t au_rate_hz   = 22050;   /* DSP 0x40 time constant -> rate */
static uint32_t au_width     = 1;       /* sample width in bytes (8- or 16-bit) */
static uint32_t au_transfers, au_stops;
static const uint8_t *au_src;
static uint32_t au_pushed;

/* 8237 state the game drives */
static uint8_t  dma_page[4];
static uint32_t dma_addr[4], dma_count[4];
static uint8_t  dma_ff[4];              /* the byte/word flip of the address / count registers */
static int8_t   dma_armed = -1;         /* channel last programmed for a transfer */

static uint8_t  dsp_wait;               /* the DSP command whose argument is still to come */
static uint16_t dsp_block;              /* 0x14/0x17/0x75/0x77 then count low, high */
static uint8_t  dsp_blk_i;

void sys_audio_set_source(const uint8_t *buf) { au_src = buf; }
uint32_t sys_audio_rate(void)     { return au_rate_hz; }
uint32_t sys_audio_width(void)    { return au_width; }
uint32_t sys_audio_transfers(void){ return au_transfers; }
uint32_t sys_audio_stops(void)    { return au_stops; }
uint32_t sys_audio_pending(void)  { return au_w - au_r; }
uint32_t sys_audio_pushed(void)   { return au_pushed; }
uint32_t sys_dma_addr(uint8_t ch)   { return ch < 4 ? dma_addr[ch] : 0; }
uint32_t sys_dma_count(uint8_t ch)  { return ch < 4 ? dma_count[ch] : 0; }
int8_t   sys_dma_armed(void)        { return dma_armed; }

static void au_put(uint8_t b) {
    uint32_t n = (au_w + 1) & (AUDIO_RING - 1);
    if (n == au_r) return;                    /* full: drop, like a card whose FIFO is overruns */
    au_ring[au_w] = b; au_w = n;
}

static void au_start(void) {
    if (dma_armed < 0 || !au_src) return;
    uint32_t n = dma_count[(uint8_t)dma_armed] + 1u;     /* the count register is programmed as n-1 */
    for (uint32_t i = 0; i < n; i++) au_put(au_src[i]);
    au_transfers++; au_pushed += n;
    dma_armed = -1;
}

static void dma8237_out(uint16_t port, uint8_t v) {
    if (port == 0x0a) {                                    /* mask / unmask */
        if (v & 4) return;                                 /* mask: not an arm */
        uint8_t ch = (uint8_t)(v & 3);
        if (ch == 3) dma_armed = 1;                        /* "unmask all": the board uses channel 1 */
        else         dma_armed = (int8_t)ch;
    } else if (port == 0x0c) {
        memset(dma_ff, 0, sizeof dma_ff);                  /* clear first/last flip flop */
    } else if (port == 0x0b) {
        memset(dma_ff, 0, sizeof dma_ff);
    } else if (port == 0x02 || port == 0x03) {
        /* The board's own DMA registers: the game writes the 24-bit transfer address as two bytes to
         * 0x02 and the byte count (n-1) as two bytes to 0x03, with the page on 0x83 (fn_040b83). */
        if (port == 0x02) {
            if (!dma_ff[1]) { dma_addr[1] = (dma_addr[1] & 0xff00u) | v; dma_ff[1] = 1; }
            else            { dma_addr[1] = (dma_addr[1] & 0x00ffu) | ((uint32_t)v << 8); dma_ff[1] = 0; }
        } else {
            if (!dma_ff[1]) { dma_count[1] = (dma_count[1] & 0xff00u) | v; dma_ff[1] = 1; }
            else            { dma_count[1] = (dma_count[1] & 0x00ffu) | ((uint32_t)v << 8); dma_ff[1] = 0; }
        }
    } else if (port >= 0x83 && port <= 0x87) {
        dma_page[(port - 0x83) & 3] = v;
    }
}

static void dsp_out(uint16_t port, uint8_t v) {
    (void)port;
    if (dsp_wait == 0x40) { au_rate_hz = 1000000u / (uint32_t)(256 - v); dsp_wait = 0; return; }
    if (dsp_wait == 0x14 || dsp_wait == 0x17 || dsp_wait == 0x75 || dsp_wait == 0x77) {
        if (dsp_blk_i == 0) { dsp_block = v; dsp_blk_i = 1; }
        else { dsp_block |= (uint16_t)((uint16_t)v << 8); dsp_blk_i = 0; dsp_wait = 0; }
        return;
    }
    switch (v) {
    case 0x40: dsp_wait = 0x40; break;
    case 0x14: case 0x17: au_width = 1; dsp_wait = v; dsp_blk_i = 0; au_start(); break;
    case 0x75: case 0x77: au_width = 2; dsp_wait = v; dsp_blk_i = 0; au_start(); break;
    case 0xd0: case 0xd1: au_stops++; dma_armed = -1; break;
    default: break;
    }
}

void sys_audio_reset(void) {
    au_w = au_r = 0; au_rate_hz = 22050; au_width = 1;
    au_transfers = au_stops = au_pushed = 0; au_src = 0;
    memset(dma_page, 0, sizeof dma_page); memset(dma_addr, 0, sizeof dma_addr);
    memset(dma_count, 0, sizeof dma_count); memset(dma_ff, 0, sizeof dma_ff);
    dma_armed = -1; dsp_wait = 0; dsp_block = 0; dsp_blk_i = 0;
}

/* drain PCM into signed 16-bit frames (mono); returns the number of frames written */
uint32_t sys_audio_fill(int16_t *dst, uint32_t frames) {
    uint32_t got = 0;
    while (got < frames && au_r != au_w) {
        if (au_width == 1) {
            uint8_t b = au_ring[au_r]; au_r = (au_r + 1) & (AUDIO_RING - 1);
            dst[got++] = (int16_t)(((int32_t)b - 128) << 8);
        } else {
            if (((au_r + 1) & (AUDIO_RING - 1)) == au_w) break;      /* a half sample: wait */
            uint8_t lo = au_ring[au_r]; au_r = (au_r + 1) & (AUDIO_RING - 1);
            uint8_t hi = au_ring[au_r]; au_r = (au_r + 1) & (AUDIO_RING - 1);
            dst[got++] = (int16_t)((uint16_t)(lo | (hi << 8)));
        }
    }
    return got;
}

/* the SB command port is the same register the base scan writes commands to (base+0xc) */
static int sb_cmd_port(uint16_t port) { return port == 0x22c || port == 0x21c; }

static uint32_t out_hist[PORT_MAX];
uint32_t sys_outb_count(uint16_t port) { return port < PORT_MAX ? out_hist[port] : 0; }

void sys_outb(uint16_t port, uint8_t v) {
    if (port < PORT_MAX) {
        out_hist[port]++;
        port_val[port] = v;
        uint8_t *hb;
        if (port == 0x3c9) {
            if (hist_n < HIST_MAX && (hb = hist_buf()) != 0) hb[hist_n++] = v;
            dac_rgb[(uint32_t)dac_index * 3u + dac_phase] = v;   /* the DAC data port */
            if (v > dac_max) dac_max = v;   /* 6-bit (0..63) or 8-bit DAC addressing? */
            dac_writes++;
            if (++dac_phase == 3) { dac_phase = 0; dac_index = (uint8_t)(dac_index + 1); }
        } else if (port == 0x3c8) {
            dac_index = v; dac_phase = 0;                        /* the DAC address port */
        }
    if (port == 0x43 || (port >= 0x40 && port <= 0x42)) pit_out(port, v);
        if (dev_hook) dev_hook(port, v);
        if (port >= 0x200 && port < 0x300 && sb_cmd_port(port)) dsp_out(port, v);
        if (port <= 0x0b || (port >= 0x81 && port <= 0x87)) dma8237_out(port, v);
        if (port >= DEV_PORT_BASE && port < DEV_PORT_BASE + 0x100) {
            dev_cmds[port]++; dev_last[port] = v;   /* the command register: writes are counted, reads are not */
        }
    }
    /* other ports: no-op (no real hardware in the port) */
}


uint8_t sys_inb(uint16_t port) {
    if (port < PORT_MAX) {
        /* a device status register answers reads independently of what was written to it */
        if (dev_seq_len[port]) {
            int i = dev_seq_at[port];
            if (i < dev_seq_len[port] - 1) dev_seq_at[port] = i + 1;
            return dev_seq[port][i];
        }
        if (dev_status_set[port]) return dev_status[port];
        return port_val[port];
    }
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
typedef struct { uint32_t off; uint16_t seg; } isr_slot;
static isr_slot   g_isr[256];
static struct { uint32_t off; void (*fn)(void); } g_isr_bind[32];

uint64_t sys_isr_get(uint8_t vector) {
    return ((uint64_t)g_isr[vector].seg << 32) | g_isr[vector].off;
}

void sys_isr_set(uint8_t vector, uint32_t handler_off, uint16_t handler_seg) {
    g_isr[vector].off = handler_off;
    g_isr[vector].seg = handler_seg;
}

uint32_t sys_isr_handler(uint8_t vector) { return g_isr[vector].off; }

void sys_isr_bind(uint32_t handler_off, void (*fn)(void)) {
    for (unsigned i = 0; i < sizeof g_isr_bind / sizeof g_isr_bind[0]; i++)
        if (g_isr_bind[i].off == handler_off || g_isr_bind[i].fn == fn) {
            g_isr_bind[i].off = handler_off; g_isr_bind[i].fn = fn;
            return;
        }
    for (unsigned i = 0; i < sizeof g_isr_bind / sizeof g_isr_bind[0]; i++)
        if (!g_isr_bind[i].off && !g_isr_bind[i].fn) {
            g_isr_bind[i].off = handler_off; g_isr_bind[i].fn = fn;
            return;
        }
}

int sys_isr_fire(uint8_t vector) {
    uint32_t off = g_isr[vector].off;
    if (!off) return 0;
    for (unsigned i = 0; i < sizeof g_isr_bind / sizeof g_isr_bind[0]; i++)
        if (g_isr_bind[i].off == off && g_isr_bind[i].fn) { g_isr_bind[i].fn(); return 1; }
    return 0;
}

void sys_isr_reset(void) {
    memset(g_isr, 0, sizeof g_isr);
    memset(g_isr_bind, 0, sizeof g_isr_bind);
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

/* Sequential stand-in stream (see sys.h). Installed by tests / the future CD backend. */
static const uint8_t *s_img = 0;
static uint32_t s_img_len = 0, s_img_pos = 0;
void sys_file_image_set(const uint8_t *img, uint32_t len) {
    s_img = img; s_img_len = len; s_img_pos = 0;
}
/* Real-file backend -- the foundation of the CD/.IMG resource layer.
 * Installing a directory makes DOS open calls resolve to real files: sys_file_open builds
 * "<dir>/<name>", and read / lseek / close then route through the FILE*. Handles are 1-based, so the
 * legacy stand-in image (handle 0) and every existing test keep working unchanged. Opening a name that
 * does not exist returns -1, which is the DOS error value the loaders (fn_04270d / fn_036853) expect. */
#define SYS_FILE_MAX 16
static FILE *s_files[SYS_FILE_MAX];
static char s_real_dir[512];
static uint32_t s_real_opens = 0;
void sys_file_real_dir(const char *dir) {
    s_real_dir[0] = '\0';
    for (int i = 0; i < SYS_FILE_MAX; i++) { if (s_files[i]) { fclose(s_files[i]); s_files[i] = 0; } }
    if (dir != 0) { snprintf(s_real_dir, sizeof s_real_dir, "%s", dir); }
    s_real_opens = 0;
}
uint32_t sys_file_real_opened(void) { return s_real_opens; }

int32_t sys_file_read(uint32_t handle, void *buf, uint32_t count) {
    if (handle >= 1 && handle <= SYS_FILE_MAX && s_files[handle - 1] != 0)   /* real file */
        return (int32_t)fread(buf, 1, count, s_files[handle - 1]);
    (void)handle;
    if (s_img == 0) return 0;                          /* no backend: 0 bytes */
    if (s_img_pos >= s_img_len) return 0;              /* EOF */
    uint32_t n = s_img_len - s_img_pos;
    if (n > count) n = count;
    for (uint32_t i = 0; i < n; i++) ((uint8_t *)buf)[i] = s_img[s_img_pos + i];
    s_img_pos += n;
    return (int32_t)n;
}
int32_t sys_file_lseek_pos = 0;   /* test knob: position a tell (whence==1) reports */
int32_t sys_file_lseek(uint32_t handle, int32_t offset, int32_t whence) {
    if (handle >= 1 && handle <= SYS_FILE_MAX && s_files[handle - 1] != 0) {  /* real file */
        int base = (whence == 1) ? SEEK_CUR : (whence == 2) ? SEEK_END : SEEK_SET;
        if (fseek(s_files[handle - 1], (long)offset, base) != 0) return -1;
        return (int32_t)ftell(s_files[handle - 1]);
    }
    /* With the stand-in image installed the position is real, so the resource loaders' seeks
     * (fn_04247a / fn_0367c9 / fn_036853) land on the per-resource body offsets of the index. */
    if (s_img != 0) {
        int32_t np = (whence == 1) ? (int32_t)s_img_pos + offset
                     : (whence == 2) ? (int32_t)s_img_len + offset
                                     : offset;
        if (np < 0) np = 0;
        s_img_pos = (uint32_t)np;
        return np;
    }
    /* no CD backend yet: a tell reports the seedable position (default 0, i.e. unchanged), so
     * tests can reproduce the invariant the real reader keeps (handle position == buffered start). */
    if (whence == 1) return sys_file_lseek_pos;
    (void)offset; (void)whence;
    return 0;   /* no CD backend: pos 0 */
}
int32_t sys_file_write(uint32_t handle, const void *buf, uint32_t count) {
    (void)handle; (void)buf; return (int32_t)count;        /* no backend: write to the void, report count */
}
int32_t sys_file_close(uint32_t handle) {
    if (handle >= 1 && handle <= SYS_FILE_MAX && s_files[handle - 1] != 0) {
        fclose(s_files[handle - 1]); s_files[handle - 1] = 0;
        return 0;                                      /* AH=3e: success */
    }
    (void)handle; return 0;
}
int32_t sys_open_ret = 0;    /* AH=3d default: success, handle 0 */
int32_t sys_create_ret = 0;  /* AH=3c default: success, handle 0 */
static int32_t s_open_seq[8];
static int s_open_top = 0, s_open_n = 0;
void sys_file_open_queue(const int32_t *vals, int n) {
    s_open_n = (n < 8) ? n : 8;
    for (int i = 0; i < s_open_n; i++) s_open_seq[i] = vals[i];
    s_open_top = 0;
}
void sys_file_open_reset(void) { s_open_top = 0; s_open_n = 0; }
int32_t sys_file_open(const char *name, uint32_t mode) { (void)mode;
    if (s_open_top < s_open_n) return s_open_seq[s_open_top++];   /* queued (test) */
    if (s_real_dir[0] != '\0' && name != 0) {                     /* real-file backend */
        char path[600];
        snprintf(path, sizeof path, "%s/%s", s_real_dir, name);
        for (int i = 0; i < SYS_FILE_MAX; i++) {
            if (s_files[i] == 0) {
                FILE *f = fopen(path, "rb");
                if (f == 0) return -1;                            /* DOS not-found error */
                s_files[i] = f; s_real_opens++;
                return (int32_t)i + 1;                            /* 1-based DOS handle */
            }
        }
        return -1;                                                /* no free handle */
    }
    return sys_open_ret; }   /* AH=3d */
int32_t sys_file_create(const char *name, uint32_t mode) { (void)name; (void)mode; return sys_create_ret; } /* AH=3c */
int32_t sys_file_delete(const char *name) { (void)name; return 0; }
uint16_t sys_fpu_control_word(void) { return 0x037f; }

struct dos_date sys_date = { 40, 1, 1, 1 };
struct dos_time sys_time = { 0, 0, 0, 0 };
struct dos_date sys_getdate(void) { return sys_date; }
struct dos_time sys_gettime(void) { return sys_time; }

/* ------------------------------------------------------------------ *
 * DPMI DOS-segment + VESA shims (used by the video-mode init 0x03f15f).
 *
 * The original allocates REAL-mode DOS paragraphs through DPMI (INT 0x31
 * AX=0x0100 code / AX=0x0110 data), stores the returned selector in the first
 * word of the block, then uses seg*16+2 as a linear pointer into it, and calls
 * INT 0x10 AX=0x4F00 / 0x4F01 / 0x4F02 in real mode through fn_00103e0 /
 * fn_0041e51. A flat process has no real mode, so the block comes out of a
 * fixed real-mode memory image and the VBE calls are answered from the tunable
 * fields below (the SDL backend would fill them from a real adapter).
 * ------------------------------------------------------------------ */
#define SYS_REALMEM 0x30000u   /* several 0x10000-byte device/DMA buffers are live at once */
static uint8_t  g_real_mem[SYS_REALMEM];

/* A DOS/DMPI program can read linear address 0 without faulting: the first megabyte stays mapped
 * (the real-mode IVT lives there). Ported code that dereferences a null table entry therefore has to
 * read something rather than trap, and what it reads decides whether the caller walks off into the
 * wild. The page is zeroed, which makes "record list terminated" the outcome. */
static uint8_t  g_linear0[64];
uint8_t *sys_linear0(void) { return g_linear0; }
static uint32_t g_real_mem_used;
static uint16_t g_real_mem_sel = 0x1000;

uint8_t  sys_vesa_signature[4] = { 'V', 'E', 'S', 'A' };  /* INT 10h/4F00h: VesaSignatureString */
uint16_t sys_vesa_mode_attributes = 0x0503;               /* ModeAttributes word */
uint16_t sys_vesa_hw_state = 1;                           /* the value the DAT_63e4c bit search looks for */
uint16_t sys_vesa_bytes_per_line = 640;                   /* ModeInfoBlock +0x10 */
uint16_t sys_vesa_xres = 640;                             /* ModeInfoBlock +0x06 / +0x12 */
uint16_t sys_vesa_yres = 480;                             /* ModeInfoBlock +0x14 */
uint16_t sys_vesa_bpp = 8;
uint16_t sys_vesa_set_mode_ah = 0;                        /* 0 = mode set ok, 1 = error */
uint32_t sys_dos_segs_allocated = 0;

/* The original releases these DPMI DOS segments again (INT 31h AX=0101h / 0301h), so the pool
 * must reclaim: fn_03f15f (the VESA mode set) spends ~0x320 bytes per call and is called again on
 * every disc-retry and mode change, which a bump-only image could not survive. A small first-fit
 * block table stands in for the DPMI allocator. */
#define SYS_REALMEM_BLOCKS 24
typedef struct { uint32_t off, size; uint16_t sel; uint8_t used; } sys_realblk;
static sys_realblk g_real_blks[SYS_REALMEM_BLOCKS];

/* the DOS-linear (paragraph-based) address of a block handed out above, i.e. what DPMI would report */
uint32_t sys_dos_seg_linear(const uint8_t *p) { return (uint32_t)(uintptr_t)(p - g_real_mem); }

/* release the block that contains p (DPMI INT 31h AX=0101); used by tests that allocate the 64 KB
 * device transfer buffer so the pool does not run dry for the next caller */
void sys_dos_seg_free_ptr(const uint8_t *p) {
    if (!p || p < g_real_mem || p >= g_real_mem + SYS_REALMEM) return;
    uint32_t off = (uint32_t)(uintptr_t)(p - g_real_mem);
    for (int i = 0; i < SYS_REALMEM_BLOCKS; i++)
        if (g_real_blks[i].used && g_real_blks[i].off == off) { sys_dos_seg_free(g_real_blks[i].sel); return; }
}

uint8_t *sys_dos_seg_alloc(uint32_t paragraphs, uint16_t *selector)
{
    uint32_t need = (paragraphs * 16u + 15u) & ~15u;              /* paragraph granularity */
    if (paragraphs == 0 || need > SYS_REALMEM) return 0;

    uint32_t off;
    int free_slot = -1;                                           /* first-fit over freed blocks */
    for (int i = 0; i < SYS_REALMEM_BLOCKS; i++)
        if (!g_real_blks[i].used && g_real_blks[i].size >= need) { free_slot = i; break; }

    int slot = -1;
    for (int i = 0; i < SYS_REALMEM_BLOCKS; i++)
        if (!g_real_blks[i].used && g_real_blks[i].size == 0) { slot = i; break; }
    if (slot < 0) return 0;                                       /* table full: DPMI would fail */

    if (free_slot >= 0) {
        off = g_real_blks[free_slot].off;                         /* carve the head of the hole */
        g_real_blks[free_slot].off  += need;
        g_real_blks[free_slot].size -= need;
    } else {
        off = g_real_mem_used;
        if (off + need > SYS_REALMEM) return 0;                   /* DPMI would fail here */
        g_real_mem_used = off + need;
    }
    g_real_blks[slot].off = off; g_real_blks[slot].size = need;
    g_real_blks[slot].sel = g_real_mem_sel; g_real_blks[slot].used = 1;
    if (selector) *selector = g_real_mem_sel++;
    sys_dos_segs_allocated++;
    return g_real_mem + off;
}

void sys_dos_seg_free(uint16_t selector)
{
    for (int i = 0; i < SYS_REALMEM_BLOCKS; i++)
        if (g_real_blks[i].used && g_real_blks[i].sel == selector) { g_real_blks[i].used = 0; return; }
}

void sys_vesa_info(uint8_t *buf)
{
    buf[0] = sys_vesa_signature[0]; buf[1] = sys_vesa_signature[1];
    buf[2] = sys_vesa_signature[2]; buf[3] = sys_vesa_signature[3];
}

/* VBE 2.0 ModeInfoBlock layout for the fields the game reads:
 *   +0x02/+0x03 ModeAttributes bytes, +0x04 hardware-state word (bit search),
 *   +0x06 XResolution, +0x10 BytesPerLine, +0x12 XResolution, +0x14 YResolution. */
void sys_vesa_mode_info(uint16_t mode, uint8_t *buf)
{
    (void)mode;
    uint16_t w;
    w = sys_vesa_mode_attributes; buf[2] = (uint8_t)w; buf[3] = (uint8_t)(w >> 8);
    w = sys_vesa_hw_state;        buf[4] = (uint8_t)w; buf[5] = (uint8_t)(w >> 8);
    w = sys_vesa_xres;            buf[6] = (uint8_t)w; buf[7] = (uint8_t)(w >> 8);
    buf[8] = (uint8_t)sys_vesa_yres; buf[9] = (uint8_t)(sys_vesa_yres >> 8);
    buf[0x0b] = (uint8_t)sys_vesa_bpp;
    w = sys_vesa_bytes_per_line;  buf[0x10] = (uint8_t)w; buf[0x11] = (uint8_t)(w >> 8);
    w = sys_vesa_xres;            buf[0x12] = (uint8_t)w; buf[0x13] = (uint8_t)(w >> 8);
    w = sys_vesa_yres;            buf[0x14] = (uint8_t)w; buf[0x15] = (uint8_t)(w >> 8);
}

static int g_vid_w = 320, g_vid_h = 200;   /* the current display geometry */
int  sys_video_w(void) { return g_vid_w; }
int  sys_video_h(void) { return g_vid_h; }

uint16_t sys_vesa_set_mode(uint16_t mode)
{
    /* VBE mode numbers the game asks for: 0x101 = 640x480x8 (fn_03f15f), 0x100 = 640x400x8. */
    if (mode == 0x101) { g_vid_w = 640; g_vid_h = 480; }
    else if (mode == 0x100) { g_vid_w = 640; g_vid_h = 400; }
    else if (mode == 0x13 || mode == 0x6f) { g_vid_w = 320; g_vid_h = 200; }
    return sys_vesa_set_mode_ah;   /* AH of the INT 10h/4F02h reply */
}

/* ---- CD-ROM (MSCDEX) backend seam: see sys.h for the contract ---- */
static sys_cd_header_fn  g_cd_header;
static sys_cd_records_fn g_cd_records;
static uint32_t          g_cd_stops;

void sys_cd_set_backend(sys_cd_header_fn header, sys_cd_records_fn records)
{
    g_cd_header  = header;
    g_cd_records = records;
}

void sys_cd_root_header(uint8_t *hdr7)  { if (g_cd_header)  g_cd_header(hdr7); }
void sys_cd_root_records(uint8_t *recs, uint32_t n) { if (g_cd_records) g_cd_records(recs, n); }

void     sys_cd_audio_stop(void)       { g_cd_stops++; }
uint32_t sys_cd_audio_stop_calls(void) { return g_cd_stops; }
void     sys_cd_audio_stop_reset(void) { g_cd_stops = 0; }

/* ---- modelled DOS / DPMI host probes (image startup 0x042044 / 0x0420bc) ---- */
static uint8_t  g_dos_major = 3, g_dos_minor = 0x1f;   /* DOS 3.31, what DOS4GW reports */
static uint16_t g_dos_marker = 0x4243;                /* 'CB': the DOS4GW marker in CX */
static uint8_t  g_dos4gw_info[0x40];                  /* the EDX info block */
static int      g_dpmi_host = 0;                      /* INT 21h AX=FF00 answer */
static int      g_dpmi_disable = 0;                   /* INT 31h AX=0006 answer */
static uint8_t  g_psp_tail[0x100];                    /* [0] = length, [1..] = characters */
static uint8_t  g_dos_env[0x400];
static uint32_t g_dos_env_len;
static uint32_t g_exit_code, g_exit_calls;

uint8_t *sys_dos4gw_info(void) { return g_dos4gw_info; }

void sys_dos_version_set(uint8_t major, uint8_t minor, uint16_t marker) {
    g_dos_major = major; g_dos_minor = minor; g_dos_marker = marker;
}

void sys_dos_version(uint8_t *major, uint8_t *minor, uint16_t *marker, uintptr_t *info) {
    if (major)  *major  = g_dos_major;
    if (minor)  *minor  = g_dos_minor;
    if (marker) *marker = g_dos_marker;
    if (info)   *info   = (uintptr_t)g_dos4gw_info;
}

void  sys_dpmi_host_probe_set(int present) { g_dpmi_host = present ? 1 : 0; }
int   sys_dpmi_host_probe(void)            { return g_dpmi_host; }
void  sys_dpmi_disable_ints_set(int count) { g_dpmi_disable = count; }
int   sys_dpmi_disable_ints(void)          { return g_dpmi_disable; }

uint8_t *sys_psp_tail(void) { return &g_psp_tail[1]; }

void sys_psp_tail_set(const char *text) {
    uint32_t n = text ? (uint32_t)strlen(text) : 0;
    if (n > 0x7f) n = 0x7f;                 /* the PSP tail is 127 characters at most */
    memset(g_psp_tail, 0, sizeof g_psp_tail);
    g_psp_tail[0] = (uint8_t)n;
    if (text) memcpy(&g_psp_tail[1], text, n);
}

uint8_t *sys_dos_env(void) { return g_dos_env; }

void sys_dos_env_set(const void *env, uint32_t len) {
    uint32_t n = env ? len : 0;
    if (n > sizeof g_dos_env - 1) n = sizeof g_dos_env - 1;
    memset(g_dos_env, 0, sizeof g_dos_env);
    if (env) memcpy(g_dos_env, env, n);
    g_dos_env_len = n;
}

uint32_t sys_dos_exit_code(void)  { return g_exit_code; }
uint32_t sys_dos_exit_calls(void) { return g_exit_calls; }
void sys_dos_terminate(uint32_t code) { g_exit_code = code; g_exit_calls++; }

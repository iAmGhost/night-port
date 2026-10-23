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
static uint32_t dac_writes;
static uint8_t  dac_index;      /* the 0x3C8 write index */
static uint8_t  dac_phase;      /* 0=R, 1=G, 2=B within the current entry */

void sys_vga_palette(uint8_t *out768) { if (out768) memcpy(out768, dac_rgb, 768); }
uint32_t sys_vga_palette_writes(void) { return dac_writes; }
void sys_vga_palette_reset(void) { memset(dac_rgb, 0, sizeof dac_rgb); dac_writes = 0; dac_index = 0; dac_phase = 0; }

/* ---- input pump hook ---- */
static sys_pump_fn input_pump;
void sys_set_input_pump(sys_pump_fn fn) { input_pump = fn; }
sys_pump_fn sys_get_input_pump(void) { return input_pump; }

static sys_pump_fn tick_pump;
void sys_set_tick_pump(sys_pump_fn fn) { tick_pump = fn; }
sys_pump_fn sys_get_tick_pump(void) { return tick_pump; }

void sys_outb(uint16_t port, uint8_t v) {
    if (port < PORT_MAX) {
        port_val[port] = v;
        uint8_t *hb;
        if (port == 0x3c9) {
            if (hist_n < HIST_MAX && (hb = hist_buf()) != 0) hb[hist_n++] = v;
            dac_rgb[(uint32_t)dac_index * 3u + dac_phase] = v;   /* the DAC data port */
            dac_writes++;
            if (++dac_phase == 3) { dac_phase = 0; dac_index = (uint8_t)(dac_index + 1); }
        } else if (port == 0x3c8) {
            dac_index = v; dac_phase = 0;                        /* the DAC address port */
        }
    if (port == 0x43 || (port >= 0x40 && port <= 0x42)) pit_out(port, v);
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
#define SYS_REALMEM 0x2000u
static uint8_t  g_real_mem[SYS_REALMEM];
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

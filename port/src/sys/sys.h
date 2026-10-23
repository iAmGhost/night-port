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
uint64_t sys_isr_get(uint8_t vector);           /* (seg << 32) | offset, 0 when unset */
void     sys_isr_set(uint8_t vector, uint32_t handler_off, uint16_t handler_seg);

/* The handlers the game installs are *image offsets* (0x3ee88, 0x41439, 0x415c9 ...). Binding an offset
 * to a ported C function makes an installed handler callable, so sys_isr_fire(vector) runs the handler
 * the game itself put in the vector -- the honest stand-in for the interrupt. */
void     sys_isr_bind(uint32_t handler_off, void (*fn)(void));
uint32_t sys_isr_handler(uint8_t vector);       /* the offset currently installed, 0 when none */
int      sys_isr_fire(uint8_t vector);          /* 1 if a bound handler ran */
void     sys_isr_reset(void);                   /* clear vectors + bindings */

/* ---- device (Sound Blaster class) port model ----
 * The game writes a command to base+0xc and polls the SAME port for bit 7, which only works because
 * the port is write-only for the command and read-only for the status. A plain port array cannot model
 * that (the write latches a byte with bit 7 set, so the poll never clears), so the read side of these
 * ports is modelled separately. */
uint8_t *sys_linear0(void);                  /* the mapped page at linear 0: what a null deref reads on DOS */
void     sys_dev_status_set(uint16_t port, uint8_t v);   /* what inb(port) answers */
uint8_t  sys_dev_status(uint16_t port);
void     sys_dev_status_reset(void);
/* successive reads of one port may answer different bytes (a device data register that returns the
 * high byte then the low byte of a word, for example); the sequence repeats its last byte. */
void     sys_dev_read_seq(uint16_t port, const uint8_t *bytes, int n);
typedef void (*sys_dev_cmd_fn)(uint16_t port, uint8_t cmd);
void     sys_dev_cmd_hook(sys_dev_cmd_fn fn);            /* called on every device write */
uint32_t sys_dev_cmds(uint16_t port);                    /* how many writes to that port */
uint8_t  sys_dev_last_cmd(uint16_t port);

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

/* Host monotonic clock (microseconds, CLOCK_MONOTONIC). The original's per-frame pacing comes from
 * spinning on hardware (the 0x3DA retrace bit), which only a host with a clock can reproduce; the
 * waits use this so they cost what they cost on real hardware instead of nothing. */
uint64_t sys_now_us(void);
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
/* Sequential stand-in resource stream: install an image whose bytes sys_file_read serves in order
 * (NULL/0 restores the historical "0 bytes" stub). Seeks stay no-ops, matching the old backend, so
 * the stream cursor only moves forward -- enough to drive the resource loaders byte-faithfully. */
void sys_file_image_set(const uint8_t *img, uint32_t len);
void sys_file_open_reset(void);                          /* test: clear the queued open results */
/* Real-file backend: install a directory whose files DOS open calls resolve to ("<dir>/<name>").
 * Handles become 1-based and read/lseek/close route through stdio; the stand-in image (handle 0) and
 * all existing tests are unaffected. Pass NULL/"" to disable (also closes any open files). */
void sys_file_real_dir(const char *dir);
uint32_t sys_file_real_opened(void);                     /* successful real opens since install */


/* DPMI DOS-segment stand-in: paragraphs -> a block in the real-mode memory
 * image, plus the selector the original stores in its first word. */
uint8_t *sys_dos_seg_alloc(uint32_t paragraphs, uint16_t *selector);
void     sys_dos_seg_free(uint16_t selector);
uint32_t sys_dos_seg_linear(const uint8_t *p);
void     sys_dos_seg_free_ptr(const uint8_t *p);  /* release the block containing p */   /* the paragraph-based address DPMI reports for a block */
extern uint32_t sys_dos_segs_allocated;

/* INT 10h AX=4F00 / 4F01 / 4F02 stand-ins (VESA VBE). Tunable for tests. */
extern uint8_t  sys_vesa_signature[4];      /* "VESA" / "VBE2"; anything else -> no VESA */
extern uint16_t sys_vesa_mode_attributes;   /* ModeAttributes word */
extern uint16_t sys_vesa_hw_state;          /* the word DAT_63e4c searches for a bit index in */
extern uint16_t sys_vesa_bytes_per_line;    /* ModeInfoBlock +0x10 */
extern uint16_t sys_vesa_xres;              /* ModeInfoBlock +0x06 / +0x12 */
extern uint16_t sys_vesa_yres;              /* ModeInfoBlock +0x14 */
extern uint16_t sys_vesa_bpp;
extern uint16_t sys_vesa_set_mode_ah;       /* 0 = mode set ok, 1 = error */
/* ---- VGA DAC (palette) model ----
 * board_out() writes the palette through outb(0x3C8, index) + outb(0x3C9, v) x3 per entry with the
 * index auto-incrementing, exactly as the VGA DAC does. The port's video backend reads the resulting
 * 768-byte RGB table with sys_vga_palette(); sys_vga_palette_dirty() counts DAC writes so a backend
 * can decide when to re-upload. */
void     sys_vga_palette(uint8_t *out768);
void     sys_vga_palette_set(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
uint32_t sys_vga_palette_writes(void);
void     sys_audio_set_source(const uint8_t *buf);  /* the buffer an armed DMA transfer reads */
uint32_t sys_audio_rate(void);                      /* the sample rate the last DSP 0x40 asked for */
uint32_t sys_audio_width(void);                     /* 1 = 8-bit PCM, 2 = 16-bit PCM */
uint32_t sys_audio_transfers(void);                 /* DMA transfers turned into samples */
uint32_t sys_audio_stops(void);                     /* DSP 0xd0 / 0xd1 stops */
uint32_t sys_audio_pending(void);                   /* bytes waiting in the output ring */
uint32_t sys_audio_pushed(void);                    /* bytes ever accepted into the ring */
uint32_t sys_audio_fill(int16_t *dst, uint32_t frames);
void     sys_audio_reset(void);
uint32_t sys_dma_addr(uint8_t ch);                  /* the 8237 address the game programmed */
uint32_t sys_dma_count(uint8_t ch);                 /* the 8237 count register (bytes = count+1) */
int8_t   sys_dma_armed(void);                       /* channel last unmasked (armed), or -1 */

uint32_t sys_outb_count(uint16_t port);    /* how many writes a given port has seen (diagnostics) */
uint8_t  sys_vga_dac_max(void);           /* largest DAC byte written: <=63 means 6-bit addressing */
void     sys_vga_palette_reset(void);

/* ---- PIT model (channels 0..2) ----
 * fn_03ef29 programs channel 0 (`outb(0x43,0x36); outb(0x40,lo); outb(0x40,hi)`) with the timer value,
 * and fn_03575d programs channel 2 the same way. sys_pit_hz(ch) returns 1193182/divisor, or 0 while
 * that channel has never been programmed. */
uint32_t sys_pit_hz(int ch);
uint32_t sys_pit_divisor(int ch);

/* ---- input pump hook (the port's asynchronous keyboard source) ----
 * The original's key state (the DAT_62c58[] scan-code slots and the DAT_63e?? held-key bytes) is
 * updated by the keyboard IRQ / the pump fn_0347e3 while the game spins. f.c's sys_key_spin() calls
 * this hook once per spin unit, so the SDL build installs a pump that pushes real key events into
 * that state; tests leave it NULL and use the scripted timeline instead. */
typedef void (*sys_pump_fn)(void);
void sys_set_input_pump(sys_pump_fn fn);
sys_pump_fn sys_get_input_pump(void);

/* Tick pump: the modern stand-in for the PIT/timer interrupt (fn_03ee88 is the installed handler).
 * A build with a tick source installs a pump here that advances real time and runs the handler the
 * right number of times, so the game's frame pacing and its busy-waits behave as they did. */
void sys_set_tick_pump(sys_pump_fn fn);
sys_pump_fn sys_get_tick_pump(void);

void sys_vesa_info(uint8_t *buf);
void sys_vesa_mode_info(uint16_t mode, uint8_t *buf);
uint16_t sys_vesa_set_mode(uint16_t mode);
int  sys_video_w(void);             /* the geometry the last mode set produced (320x200 until one) */
int  sys_video_h(void);

/* --- CD-ROM (MSCDEX) backend -------------------------------------------------
 * The original reached MSCDEX through DPMI real-mode call structures (fn_10570 /
 * fn_107b9 / fn_041e51), all inert in a flat process. The disc-present gate fn_010c3e
 * depends on the two buffers that call filled, so a modern backend can be installed:
 *   header(hdr7)    fills the 7-byte root-directory header at the far buffer DAT_00053c5c;
 *                   fn_010927 then derives the entry count DAT_53c60 = hdr[2] - hdr[1] + 1.
 *   records(recs,n) fills the n 5-byte records of the DAT_00053c64 array; fn_010c3e compares
 *                   the raw dwords at recs+0xb and recs+6 (the dword at +1 of records 2 and 1)
 *                   and requires the difference 0xc51 (3153 bytes).
 * With no backend installed both entry points do nothing, which is the inert DOS behaviour
 * the existing CD-family tests pin. */
typedef void (*sys_cd_header_fn)(uint8_t *hdr7);
typedef void (*sys_cd_records_fn)(uint8_t *recs, uint32_t n);
void sys_cd_set_backend(sys_cd_header_fn header, sys_cd_records_fn records);
void sys_cd_root_header(uint8_t *hdr7);
void sys_cd_root_records(uint8_t *recs, uint32_t n);

/* The MSCDEX "stop audio" request behind fn_000405a7 (real-mode request 0x13). Counted so
 * callers become observable in tests. */
void     sys_cd_audio_stop(void);
uint32_t sys_cd_audio_stop_calls(void);
void     sys_cd_audio_stop_reset(void);


/* ---- modelled DOS / DPMI host probes for the image startup (0x042044 / 0x0420bc) ----
 * The startup interrogates the host before anything else runs, so the host behaviour is
 * here rather than in the game layer. Defaults reproduce a DOS4GW client (INT 21h AH=30
 * answering DOS 3.31 with the extenders marker CX=0x4243 and EDX = its info block). */
void      sys_dos_version(uint8_t *major, uint8_t *minor, uint16_t *marker, uintptr_t *info);
void      sys_dos_version_set(uint8_t major, uint8_t minor, uint16_t marker);
uint8_t  *sys_dos4gw_info(void);            /* the EDX info block: +0x10 PSP base, +0x1c DTA, +0x2c DS paragraph */
void      sys_dpmi_host_probe_set(int present);   /* INT 21h AX=FF00 DX=78: 0 = no DPMI host */
int       sys_dpmi_host_probe(void);
void      sys_dpmi_disable_ints_set(int count);   /* INT 31h AX=0006 -> CX = previous disable count */
int       sys_dpmi_disable_ints(void);
uint8_t  *sys_psp_tail(void);               /* PSP+0x81 command tail; the length byte is at [-1] */
void      sys_psp_tail_set(const char *text);     /* stores the length byte + characters */
uint8_t  *sys_dos_env(void);                /* the NUL-separated, double-NUL-terminated env block */
void      sys_dos_env_set(const void *env, uint32_t len);  /* NUL-separated entries; len counts the bytes to store */
uint32_t  sys_dos_exit_code(void);          /* INT 21h AH=4C: the exit code the program ended with */
uint32_t  sys_dos_exit_calls(void);
void      sys_dos_terminate(uint32_t code);

#endif

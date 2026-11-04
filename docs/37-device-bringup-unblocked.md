# The audio/device bring-up now actually runs (0x040d09 chain), plus the DPMI block allocator

Three port-layer gaps that the earlier sessions had documented as blockers are closed. Together they let
the game's own device bring-up (`fn_040d09`, called from the master bring-up `fn_03658d`) complete, which is
the precondition for any sound path in the port.

## 1. The status port had to be separated from the command port

`fn_041340(cmd)` writes to `base+0xc` and `fn_041390()` polls `inb(base+0xc)` for bit 7 **clear**; on a real
card that port is write-only for the command and read-only for the status. A plain port array cannot model
that — the written command byte (e.g. `0xf2`) has bit 7 set, so the poll never cleared and the chain died at
the presence probe. `src/sys/sys.c` now models device ports separately:

* `sys_dev_status_set(port, v)` — what `inb(port)` answers;
* `sys_dev_read_seq(port, bytes, n)` — successive reads return successive bytes (the id register answers a
  high byte then a low byte), repeating the last;
* `sys_dev_cmd_hook(fn)` + `sys_dev_cmds(port)` / `sys_dev_last_cmd(port)` — the write side is recorded for
  ports `0x200..0x2ff` (the range `fn_040f4f`'s base scan probes).

## 2. Interrupt vectors are real now, and handlers are callable

`sys_isr_get/sys_isr_set` were no-ops, so every "save the old vector, install ours" step was invisible. They
now keep a 256-entry `(offset, seg)` table, and:

* `sys_isr_bind(image_offset, c_function)` maps the *image offsets* the game installs (`0x3ee88`, `0x3fd40`,
  `0x41439`, `0x415c9`) onto ported C functions;
* `sys_isr_fire(vector)` runs whatever the game itself installed.

That is how the device probe gets its interrupt: the test's command hook fires vector `0x0a` when `0xf2` is
written, which calls `fn_041439`, which sets `DAT_00063f84 = 2` — so `fn_040fb3` returns 2 through the
handler instead of running its `0x7fff` countdown to zero. Two older test expectations that had pinned the
stub behaviour were upgraded: `fn_03ef29` really leaves `0x3ee88 : 0x1f` in INT `0x1c`, and `fn_03feb8` really
leaves `0x3fd40` on IRQ 9.

## 3. `0x040788` (DPMI allocate DOS memory block) is no longer a stub

Disassembly of `0x40788`: a request struct of `[0] = 0x100` (the INT 31h function number) and
`[4] = (size + 0xf) >> 4` paragraphs handed to the `fn_041e51` trampoline; on success the returned block
segment is stored through the EAX output slot and the function returns `(segment & 0xffff) << 4`. The port
answers it from the real-mode memory image (`sys_dos_seg_alloc`), which grew from 0x2000 to 0x20000 bytes
because the device transfer buffer alone asks for 0x10000. New: `sys_dos_seg_linear(p)` gives the paragraph
address DPMI would report; `DAT_00063fb0` holds the segment `fn_00040788` stores.

## End-to-end result

`test_device_bringup` (9 checks) runs `fn_040d09` against the model and pins: base found at `0x220`
(`fn_040f4f` returns the base whose status reads `0xaa`; note `0x210` is the *not-our-controller* value the
caller rejects), presence `2` reached through the fired interrupt, the command stream `0xf2`/`0xe1`/`0xd1`
written to `base+0xc`, the id read as two data bytes, ISR `0x415c9` installed in the vector table,
`DAT_00063f90 = 1`, the 64 KB transfer buffer allocated, and `DAT_00063fac` carrying the ISA DMA address.

Suite: **2775 PASS / 0 FAIL**, ASan clean, `make selftest` ALL PASS, and the SDL host still boots the
real data, passes the disc gate and renders a stage (`--autokey`, 640x480, 256 colours, 3 entities).

## Still open on the visuals

The host now scales DAC values by 4 only when the largest byte written to `0x3c9` is ≤ 63 (`sys_vga_dac_max`)
— the stage push uses full 8-bit values, so that path is not the washed-out look. The stage frame is laid out
correctly (two character sprites, HUD frame, plane structure) but the palette applied to it is near-white, so
the next step is a side-by-side against the original under dosbox-staging, which separates "the wrong palette
is pushed for this room" from "the background plane indices come from the wrong stream".

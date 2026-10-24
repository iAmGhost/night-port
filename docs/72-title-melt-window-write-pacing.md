# 72. The melting title: what actually paces it, and the port/original duration ratio

Closes the `title-pacing` playtest task (and with it log item 20, the logo hold). The animation the playtest report
called "plays too fast" is not a counter bug in the game: it is paced by the cost of writing through the A0000 VGA
window, which the flat model made free.

## The animation, as shipped

`fn_03a224` (0x0003a224) is the title screen. After loading `title3.pcx`, `title4.pcx` and `title1.pcx` it runs the
wipe:

    0x3a2d7  mov  dword [ebp-0x10], 0        ; row = 0
    0x3a2e0  add  dword [ebp-0x10], 2        ; row += 2   (so 0x1e0/2 = 240 passes, confirmed by disassembly)
    0x3a2e4  cmp  dword [ebp-0x10], 0x1e0

Every pass composites into a scratch buffer and then calls `fn_03f462(0, 0, 0x280, 0x1e0, scratch)`, i.e. puts
**0x280 x 0x1e0 = 307200 bytes through the A0000 window per pass** (240 passes = 73.7 MB of window writes).

## The original's number

From the clean 6 Hz raw capture of the original's boot (`/tmp/dbgauto/orig_intro_raw`, docs/71 basis, decoded with
the per-mode channel order of docs/70), classifying every 640x480 frame by a gain-fitted NCC against the shipped
`title1.pcx` / `title3.pcx`:

    48387-50132 ms  black (title window, nothing drawn yet)
    50308 ms        first partially-resolved frame (NCC 0.385)   <- wipe starts
    ...             NCC climbs 0.39 -> 0.54 -> 0.58 -> 0.90 while the fitted gain wanders 0.37-0.84
    57497 ms        first frame matching the settled card (NCC 0.959)  <- wipe ends
    57497-77104 ms  title held at fitted gain ~0.79

**Wipe span = 7190 ms = 29.96 ms per pass = 97.5 ns per window byte** -- about two 486 cycles per uncacheable VGA
write, which is exactly what the hardware costs.

## First hypothesis, and why it was wrong

The obvious candidate was the palette call: `fn_03f462` calls `fn_03f3be` (INT 10h AX=0x4F09, VBE Set Palette
Register) and passes the *row number* as the colour, so I charged 31 us per call and wrote a test that a full-height
write issues 0x1e0 calls. It passed the test and was still wrong: with the game's real geometry
(`g_dat_531f4 = 640<<10`, `g_dat_63e64 = 0x280`, both from the VBE mode-info block) the row-advance branch inside
`fn_03f462` -- the one that calls `fn_03f3be` -- almost never fires, so the real number of issued VBIOS calls per
pass is ~1, not 481. The test had pinned an artificial configuration (W = pitch = 0x280). Reverted.

Second trap, worth recording: `make -C port test` builds `test/test_f1` but **not** `test/night`, so the first two
"measurements" of the fix ran the old binary and showed no change at all. `make -C port app test` (or touching the
sources) is required before measuring through `test/night`.

## The fix

`g_vga_window_ns` in f.c: a modelled per-byte cost for writes through the A0000 window, charged once per scanline
inside `fn_03f462`, with a byte counter (`g_vga_window_bytes`) for tests and diagnostics. Like the retrace wait and
`delay_units`, it waits through `sys_port_spin()` so the cooperative tick/input pumps keep running, and it stays a
no-op when no pump is installed (every unit test), so the suite remains timing-free.

The charge is kept in a debt accumulator with the overshoot credited back, because a single `sys_port_spin()` can run
past the deadline by tens of microseconds and 480 fresh per-row waits per pass then drift long.

The host default is **63 ns/byte, not the physical 97.5**, and that is deliberate and measured: at 98 ns/byte the
port's wipe measured 11129 ms (ratio 1.548) because the pump's wait granularity and the port's own compositing work
add roughly 11 ms per pass on top of the charge. Tuning the charge so the observed span lands on the shipped number
is what the contract asks for; the physical figure is still pinned in the test as the reference.

## Result

Port, same route with no key input (`scripts/playtest/route_attract_wait.txt`, `NIGHT_DENSE=1`, 5 Hz card shots):

    first partially-resolved frame  t=47906 ms
    first frame matching the card   t=55168 ms
    span = 7262 ms ; 51 intermediate wipe frames sampled
    original span = 7190 ms  ->  **port/original duration ratio 1.010**

Before the change the whole wipe fitted inside one 200 ms shot interval (no intermediate frames at all).

## Side effect that closes log item 20 (the logo hold)

The logo card is written through the same window, so its hold came out right too. Same run, measured against a
`logo.pcx` decode with a per-frame gain fit:

    PORT     visible span 2289 ms, plateau (>= 90 % of peak gain)  879 ms, gains 0.184/0.400/0.641/0.819/0.962 x5
    ORIGINAL visible span 2283 ms, plateau                          878 ms, gains 0.122/0.365/0.629/0.850/0.970 x6

The earlier shortfall (port 1800 ms visible / 400 ms plateau) is gone; the fade ladder and the dwell now agree with
the original to within a few milliseconds.

## Tests

`test_title_wipe_window_cost` in `port/test/test_f1.c` pins: one full-height `fn_03f462` write charges exactly
0x280*0x1e0 = 307200 window bytes under the game's real geometry; at the physical 97.5 ns/byte the 240 passes come to
the original's measured 7.19 s; the host default charges 4.65 s of it (the pump supplies the rest); the measured
port/original span ratio 1.010 stays inside +-3 %; and the suite keeps the model timing-free.

Suite after the change: `make -C port test` ALL PASS, `make -C port selftest` ALL PASS.

## Not claimed

- The window-write charge is applied in `fn_03f462` only (the slide/title writer). Other writers into the arena -- the
  room renderer's page flip and the sprite blitter -- are not charged yet, so their pacing is still modelled only by
  the retrace wait and `delay_units`. That is the natural next step and is filed in the playtest log.
- The port's title settles at fitted gain ~0.997 against the card while the original settles at ~0.79; that palette
  level difference is separate from pacing and still open.

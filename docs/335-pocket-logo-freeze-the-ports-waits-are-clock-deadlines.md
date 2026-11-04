# 335 — The hardware run stops on the company logo: where the port waits, and which clock it waits on

First real-Pocket run (manifest fixed, docs/334 §7): the core loads, the boot chain runs, the company logo
appears and **fades in**, and then the picture sits there -- no fade-out, no opening movie, no title menu.
That freeze point is exact enough to name in the port's own code.

## 1. Where it stopped

`fn_038a20` is the company-logo screen (f.c:16744):

```c
fn_03f15f();                                       /* VESA mode set */
fn_041e20(g_dat_633b8, 0, 0x300); board_out(...);  /* DAC to black */
fn_03628e("logo.pcx", 0);                          /* the picture */
fn_040660(g_dat_636b8, 0x32);                      /* 50-frame fade-IN   -- this ran */
for (int32_t i = 0; i < 0x400; i++) delay_units(1);   /* the hold  -- this is where it stays */
fn_0406f5(g_dat_636b8, 0x32);                      /* 50-frame fade-OUT -- never reached */
for (int32_t i = 0; i < 0x400; i++) delay_units(1);
```

So the device got through the mode set, the PCX decode, the palette push and 50 retrace-paced fade frames --
which already proves the display path, the palette path and the tick pump are alive.  What it did not get
through is 1024 iterations of `delay_units(1)`, and that function is nothing but a clock deadline
(f.c:11414):

```c
uint32_t v = arg * g_clock_ticks + 0x1f4;          /* 1*18 + 500 */
uint32_t es_i = v / 0x3e8;                          /* /1000 -> 0, clamped to 1 */
if (es_i == 0) es_i = 1;
while (es_i-- > 0) {
    sys_clock_read();
    uint64_t until = sys_now_us() + g_delay_unit_us;         /* 500 us */
    while (sys_now_us() < until) { if (!sys_port_spin()) break; }
}
```

Every wait in this port has that shape -- `delay_units`, `fn_00035ec2`'s retrace stand-in
(`while (sys_now_us() < next_us)`, 14286 µs per fade frame, f.c:906), the VBLANK busy-waits at f.c:16170 and
14646 -- and `sys_port_spin()` returns 1 whenever a pump is installed, so such a loop only ends when
**`sys_now_us()` advances past the deadline**.  A clock that does not advance in microseconds does not make
the port crash; it makes the port stand exactly where the user saw it standing.

## 2. The clock the Pocket build was using

`src/sys/sys.c` had one implementation:

```c
uint64_t sys_now_us(void) {
#ifdef CLOCK_MONOTONIC
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) ...
#endif
    return (uint64_t)clock() * 1000000ull / (uint64_t)CLOCKS_PER_SEC;
}
```

openfpgaOS's libc spells its free-running clock `clock_us()` / `clock_ms()` in `<time.h>` and the SDK hands
it out as `of_time_us()` / `of_time_ms()` (of_timer.h: "For time queries use clock_gettime() / clock_ms() /
clock_us()"), not as `CLOCK_MONOTONIC`.  So the RISC-V build was on the last-resort `clock()` branch -- a
libc stub on a bare-metal target.  The desktop build was never affected: macOS has `clock_gettime`, and the
measured rate there is `clock=1001640` µs per wall second.

## 3. The fix: an installable clock source

`src/sys/sys.h` / `sys.c` gain one seam (additive; no behaviour changes for the SDL host or the fixture
suite, which keeps the old path because it installs nothing):

```c
typedef uint64_t (*sys_clock_fn)(void);
void sys_set_clock_source(sys_clock_fn fn);

uint64_t sys_now_us(void) {
    if (sys_clock_src) return sys_clock_src();
    ... the old CLOCK_MONOTONIC / clock() path ...
}
```

and `pocket/src/night/main_of.c` installs the OS counter, carrying its 32-bit wrap (4295 s) into the port's
64-bit clock:

```c
static uint64_t of_clock_us(void) {          /* of_time_us() is 32-bit microseconds */
    uint32_t v = of_time_us();
    if (v < g_clock_last) g_clock_base += 0x100000000ull;
    g_clock_last = v;
    return g_clock_base + v;
}
sys_set_clock_source(of_clock_us);           /* installed before anything else runs */
```

Two diagnostics go with it, both readable on the device through `Pocket LCD -> Terminal` (docs/334 §6):

```
clock: openfpgaOS of_time_us installed; probe: 20137 us over a 2M-iteration spin (advancing)
memstat frames=… ticks=…(+491/s) pumps=… … clock=1000003
```

The probe at boot answers the only question that matters here -- does the port's clock advance at all -- and
the per-second `clock=` field is the measured clock microseconds per wall second: about `1e6` is correct,
anything two or more orders of magnitude off means the waits run at the wrong pace.

## 4. What the next hardware run has to report

| what the Terminal shows | reading |
|---|---|
| logo fades in **and out**, opening movie plays | the clock was the fault |
| `… (NOT ADVANCING)` in the probe line | `of_time_us()` is not usable here; the host needs a different source (`clock_us()` directly, or `of_timer_set_callback` counted by hand) |
| `clock=` far from 1 000 000 | the clock runs but at the wrong rate; the wait *durations* (500 µs, 14286 µs) need the rate corrected, not the source |
| log lines keep scrolling with `ticks=+490/s` yet the picture stays frozen | the hold loop is fine and something in the *presentation* path stopped; bisect with `NIGHT_OFF`/`cdda` first |
| the log stops dead at a line | the app died there: the last line names the phase (`board_boot` / `audio_boot` / `cdda_boot` / `disc:` / a `memstat`) |

Repo fixture suite after the sys.c seam: `make test` -> `== ALL PASS ==`.  Desktop Pocket shim after it:
`frames presented: 900 … clock=1001640` unchanged.

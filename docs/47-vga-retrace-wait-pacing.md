# 47 — the missing pacing source: the 0x3DA retrace wait was a stub

Four of the reported playtest issues turned out to have one cause: **`fn_00035ec2` (0x035ec2) was ported
as an empty function**, so every per-frame wait in the game cost nothing.

The original spins on the VGA status port: read `0x3DA` until the retrace bit clears, then until it is
set again — one monitor frame per call. It sits inside the palette-fade pair `fn_040660` / `fn_0406f5`
(one call per fade frame, 0x32 and 0x33 frames), inside the screen loaders (`fn_03628e` chain), and in
the boot/attract sequence. So a logo hold, a slideshow card, or a melting title is *paced by fades*, not
by `delay_units`.

`delay_units` (0x04564f) is genuinely cheap in the original — worth recording, because it looks like a
delay:

```
00045675  xor eax,eax ; mov bl,al
00045679  mov ah,0x2c ; int 0x21      ; DOS get time, DH = hundredths
0004567d  cmp bl,dh                   ; result NEVER used
0004567f  dec esi
00045680  jne 0x45679                 ; loop count = esi = ceil((arg*ticks+500)/1000)
```

With `arg = 1` that is a single `INT 21h/2Ch`, so the game's `0x400 × delay_units(1)` "waits" are
effectively free — the visible hold time comes from the retrace waits in the fades around them.

## Fix

`fn_00035ec2` now waits for the next retrace boundary against the host clock (`sys_now_us()`, new in
`src/sys/sys.c`), cooperating with the installed tick/input pumps like the other busy-waits in the port:

```c
if (!sys_get_tick_pump() && !sys_get_input_pump()) return;   /* unit tests: timing-free stub */
next_us += g_vga_retrace_us;                                /* 14286 us = 70 Hz mode-13h retrace */
while (sys_now_us() < next_us) { if (!sys_port_spin()) break; }
```

* Host default 70 Hz (`NIGHT_RETRACE_HZ=<n>` to override, logged at startup, `retrace=` counter in the
  pump diagnostics).
* With **no pump installed** (every unit test) it stays the stub the tests were written against, so the
  verification suite keeps its timing-free behaviour.

## Measured, same route script (`scripts/playtest/route_episode1.txt`)

| | before | after |
|---|---|---|
| first resource the game opens | `opening.fli` (logo never shown) | `logo.pcx`, held ~1.4 s before the movie |
| episode-1 introduction slides | none | `1s1.pcx` → `1s2.pcx` → `1s3.pcx` (0.7 s each) then `stage1.pcx` |
| `.fli` intro frames sampled at 2 Hz | 0 (whole boot finished inside one presented frame) | 20 samples across ~9 s |
| retrace waits run | 0 | 621 by t=33 s |

That closes the "no company logo" and "no introduction slideshows" items outright and is the reason the
title/attract animation played too fast; the remaining reported items (intro artefact, stage-transition
corruption, enemy/player sprites, health-bar position, audio) are tracked separately in
`docs/manual-playtest/00-2026-10-23.md`.

## Regression test

`test_vga_retrace_wait` (4 checks): the wait must stay timing-free with no pump installed, and with a
1 ms retrace installed it must cost ≥20 ms over 25 calls while running the installed tick pump.

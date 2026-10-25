# 78. The host debug-warp crash, and the pre-existing overrun it was hiding

Closes the docs/75 harness caveat: *"`NIGHT_SHOWSCREEN=gameover NIGHT_SHOWSCREEN_AT=1500` segfaults (exit 139) ... the
host warp should be guarded on the game having actually started (filed, not fixed)."*

## The crash, reproduced under ASan

    cc -fsanitize=address ... -o test/night src/app/main.c src/f/f.c ...
    NIGHT_SHOWSCREEN=gameover NIGHT_SHOWSCREEN_AT=1500 ./test/night --script route_attract_wait.txt ...

Before the fix (both routes, ASan build):

```
showscreen warp: gameover at frame 1525
ERROR: AddressSanitizer: SEGV on unknown address 0x000000000048
  #0 fn_0406f5            f.c:288        local[j] = arg1[j] * i / arg2
  #1 fn_038445            f.c:24208      fn_0406f5(g_dat_63354 + 0x48, 0x32)
  #2 tick_pump            main.c:1050    the NIGHT_SHOWSCREEN warp
```

Root cause, straight from the shipped code: `fn_038445` (game over / CONTINUE) *opens* with
`fn_0406f5(DAT_63354 + 0x48, 0x32)` -- a 50-step DAC fade-out of the **current room's** palette. In the game its only
call site (`0x0003aecf`) sits inside the room play loop, so `DAT_63354` is always a live room record there. Warping to
it during the title/attract phase, before any room is loaded, dereferences address `0x0 + 0x48`.

## The guard

`fn_showscreen_gate(what)` in `port/src/f/f.c` states the shipped precondition; `port/src/app/main.c` now **defers**
the warp instead of firing it (and logs the reason once), so the warp still happens as soon as the state exists:

```
case A  title/attract only (route_attract_wait), AT=1500:
        showscreen warp: gameover deferred at frame 1532: no current room record
        (DAT_63354 == 0): fn_038445 fades the room palette          -> exit 0, never fires
case B  --warp-stage 1 (a room is loaded by the host warp), AT=1500:
        showscreen warp: gameover deferred at frame 1533: ...
        showscreen warp: gameover at frame 2572                      -> exit 0, fires once legal
```

The `SEGV` count in the ASan log is now 0. Regression test `test_showscreen_gate` (port/test/test_f1.c): refused while
`g_dat_63354 == 0`, the refusal names the missing shipped state, allowed once a room record exists, the ending slide
show is not gated (it has no room precondition), an unknown screen is left to the caller. Suite and selftest ALL PASS.

## Separate pre-existing defect found by the same ASan run (filed, root cause identified)

With the SEGV gone, the ASan run stops at a different, older problem:

```
ERROR: AddressSanitizer: global-buffer-overflow ... READ of size 4
  #0 fn_03f6bc_impl  f.c:13625     if ((uint32_t)(uint16_t)idx == g_tab_53200[cursor])
  #1 fn_03f6bc       f.c:13596
  #2 fn_038ae0       f.c:14927
  #3 fn_03af18       f.c:26196
0x...ef4 is located 0 bytes after global variable 'g_tab_53200' (0x154 bytes)
   and 44 bytes before 'g_dat_53338'
```

Root cause: `fn_03f6bc_impl` walks the timed-event table with `cursor` incremented on every match and never bounded,
and reads `DAT_53200[cursor]` past the table's 61 dwords. In the shipped data segment that is *not* out of bounds:
`DAT_00053200 + 61*4 == DAT_000532f4` exactly (recorded in `f.h:836`), so the original reads the start of the next
table, `DAT_000532f4`. The port models the two tables as separate C globals with compiler-chosen padding between them,
so the same unbounded read lands in padding instead of `DAT_000532f4` -- a modelling gap, not a transcription error.
Both tables also have *no producer inside the EXE* (`seedable`, awaiting the CD backend), which is why nothing in the
port ever keeps `cursor` inside range.

Fix shape (**since applied**, in docs/83 -- kept as a separate change because it touches the data-segment model): lay
`DAT_53200` and `DAT_532f4` out in one contiguous block covering `0x53200..0x53338` and index through it, exactly as
the original's DS layout does; then the overrun read returns what the original would read. docs/83 measures the walk
reaching cursor 61 in every run (so the aliased read does happen), with 0 ASan reports and 0 clamps. Before that
change ASan reported it in any long title/attract run, and it was *not* the warp crash docs/75 filed.

## Reproduce

    make -C port test                     # includes test_showscreen_gate
    make -C port app
    cd port && NIGHT_SHOWSCREEN=gameover NIGHT_SHOWSCREEN_AT=1500 \
      ./test/night --script ../scripts/playtest/route_attract_wait.txt --no-audio --frames 3706
    # and with --warp-stage 1 to see the warp fire at the first legal frame

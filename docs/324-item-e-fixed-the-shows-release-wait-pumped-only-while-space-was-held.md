# 324 — Item E fixed: fn_03a70e's release wait pumped only while Space was held, so a held Enter survived into the next picture

Basis for both halves: `--warp-stage 1 --warp-column 3` (stage 1's last playarea — node #4, col 3, the only
node whose `+0x34c` is NULL) with `scripts/rig/drives/ep1_st1_room3_walk.txt`, `--frames 14000 --rig-hz 1`,
`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, `NIGHT_SHOWDBG=1` (`stdbuf -o0 -e0`).
Localisation and the red census are docs/323.

## 1. The defect

`fn_03a70e`'s release wait, transcribed as

```c
do { while (g_dat_63ec1 != 0) { sys_port_spin(); } } while (g_dat_63ea4 != 0);
```

pumps **only inside the inner `while`**.  The shipped's two tests (0x3a7cd tests `DAT_63ec1`, 0x3a7d6 tests
`DAT_63ea4`, 0x3a7dd exits when both are clear, 0x3a7df loops) contain no store — the shipped's latch bytes
are cleared asynchronously by the keyboard handler while the CPU spins.  In this port the only writer of those
bytes is the keyboard-handler model (`port/src/app/keymap.c`: make at 0x0003fe06, break at 0x0003fe69-6e),
and it runs only when a pump runs.  So with **Space clear and Enter still held** — exactly what a press
leaves behind — the port's wait loops with *no pump at all*: nothing can ever clear `DAT_63ea4`.

Two consequences, both measured:

- The latch survived into the next picture's gate.  Because the wait's only exits came from *incidental*
  pumping elsewhere in the picture cycle (`fn_04185e` at 0x3a7c8, `fn_036041`'s picture load), a picture could
  be consumed by a press that its own gate never waited for: red = **5 pictures, 4 gates satisfied with no
  press** (`/tmp/rig/e325r`, docs/323 §3).
- The wait can stop being exitable.  The same drive with the new witnesses hung inside picture 1's release
  wait and had to be killed: `/tmp/rig/e325s` ends at
  `show: pic 1 press taken by enter(63ea4) (gate spins 178567)` and never prints the release-wait line, with
  no frames advancing (the frame-budget escape also lives in the pump, so an un-pumped wait suspends the run
  outright).

## 2. The fix (predicate unchanged, pump per iteration)

Factored out as `f_show_release_wait()` (`port/src/f/f.c`, declared in `f.h`), in the same shape the port
already uses for the title/menu drains (`fn_03a224`'s drain at 0x3a409-431, `fn_038cde`'s at 0x38edc-f21):

```c
unsigned f_show_release_wait(void)
{
    unsigned spins = 0;
    while (g_dat_63ec1 != 0 || g_dat_63ea4 != 0) { sys_key_spin(); spins++; }   /* 0x3a7cd / 0x3a7d6 */
    return spins;
}
```

`fn_03a70e` calls it in place of the nested `do/while`.  The shipped's condition (both `DAT_63ec1` and
`DAT_63ea4` clear to proceed) and its order (ec1 tested first) are kept; only the pump placement changes,
and the press gate at 0x3a789-9f already pumped every iteration.

Nothing was added to the gate: no rising-edge test, no clearing store, no new latch.  The shipped's gate at
0x3a785-0x3a7a4 really does break on a held latch (docs/323 §2), and that is kept.

## 3. Green on the identical drive and basis (`/tmp/rig/g325a`)

```
show: entry stage=1 count=5 latches esc=0 enter=0 ... pumpoff=1
pic 1/5 pre-latch enter=0 space=0 esc=0  -> press taken by enter(63ea4) (gate spins 137668)  release wait spun 40832
pic 2/5 pre-latch enter=0 space=0 esc=0  -> press taken by enter(63ea4) (gate spins 609072)  release wait spun 44960
pic 3/5 pre-latch enter=0 space=0 esc=0  -> press taken by enter(63ea4) (gate spins 599911)  release wait spun 44149
pic 4/5 pre-latch enter=0 space=0 esc=0  -> press taken by enter(63ea4) (gate spins 622864)  release wait spun 38214
pic 5/5 pre-latch enter=0 space=0 esc=0  -> press taken by enter(63ea4) (gate spins 615832)  release wait spun 44891
```

Counted census on the same stream (`press:` lines, one per drive key event): 17 counted Enter press events in
the run, **5 picture gates each starting at `enter=0` and each waiting for its own press**, gates satisfied
with **no press = 0**, every release wait ending at `post-latch enter=0 space=0` ✓

| | red `/tmp/rig/e325r` | green `/tmp/rig/g325a` |
|---|---|---|
| pictures consumed | 5 | 5 |
| gates satisfied with no press | **4** | **0** |
| gates that waited for a press (spins > 0) | 1 | **5** |
| release wait that pumped | 0 lines emitted (structure had no counter) | 5, each 38k-45k iterations, all ending 0/0 |
| run completed | yes | yes |

## 4. Witnesses added (they are what made the mechanism visible)

- `NIGHT_LATCHDBG=1` — writer census on every latch-byte change (`keymap.c`): `latchwrite sc=1c make=1
  idx=1c before=0`.  This is what proves the byte only moves through the keyboard-handler model.
- `press: sc=%02x down|up t=%ldms` (and `typematic-repeat`) printed to **stderr** in `pt_drive`, so the press
  census and the `show:` witness share one ordered stream (they were previously split across stdout/stderr,
  which is why the earlier run could not attribute presses to pictures).
- per-wait spin counts in `fn_03a70e` (`gate spins`, `release wait spun`).

## 5. Fixture

Three checks added to `test_fn_03a70e` (`port/test/test_f1.c`), driving the drain with the scripted key
timeline (`sys_key_script_add`) so the release lands on a known spin:

```
[PASS] an Enter-only hold is drained: the wait pumps while only DAT_63ea4 is held      (spins == 2, byte cleared)
[PASS] a held DAT_63ec1 is drained the same way                                         (spins == 1, byte cleared)
[PASS] with both latches clear the wait does not spin at all                            (spins == 0)
```

`make -C port test` -> `== ALL PASS ==`, 0 FAIL.

## 6. What this does and does not close

Item E ("one Enter press skips the rest of the dialogue") is closed as a **pump-placement defect in the
port's key waits**, not as a gate-logic difference: the gate and the wait predicates are instruction-faithful
to 0x3a785-0x3a7df, and after the fix a picture can only be consumed by a press that its own gate waited for.
The reveal wipe that the same show runs at the end (`fn_03a5f7` at 0x3a7e7-f0) is item F, and is attacked next
on this basis.

## 7. Playtester re-test status, recorded 2026-11-04

Item E's closure is **rig-gated**, not playtest-gated: red `/tmp/rig/e325r` (5 pictures consumed, 4 gates
satisfied with no press, docs/323), green `/tmp/rig/g325a` (0 such gates, each wait 137k-622k spins), re-checked
on the final HEAD at `/tmp/rig/e330g` (`show: entry stage=1 count=5`, five `release wait spun` lines,
`press taken by enter(63ea4) (gate spins 400565)`, zero gates satisfied without a press).

A human re-test of this exact moment is **still outstanding and is not claimed here**.  The 2026-11-04
playtester confirmation on file (docs/325 §5: "stage 2->3 graphical glitch no longer present", "stage 1-3 boss
screen effect appears now") covers the reveal, i.e. item F, not the dialogue advance.  The run being asked for,
so the gate has a witness outside the rig: get into map 1's last room (column 3 -- `--warp-stage 1
--warp-column 3` installs the same room the rig measured), let the overlay dialogue start, and press Enter once
at a time.  Expected after the fix: one picture per press, and no picture consumed by a latch carried from the
previous one.

# 416 — the Pocket's holds are long because the delay loop presents a 640×480 surface 57 times a second

docs/415 measured the original's stage-card hold at 3.91 s of real time against the Pocket's 7.95 s and called the
per-DOS-clock-read cost the lever. It is not the clock read. This closes the gap's arithmetic, and the number that
comes out of it is the same one the choppy intro has been waiting on.

## The clock read is free

`src/sys/sys.c:442` — `void sys_clock_read(void) { sys_clock_read_count++; }`. A counter. Whatever is stretching
the hold, it is not emulating a DOS clock read, on either host.

## What the hold costs, from the card log of the 0.1.93 boot

The hold is the window between `cdda EVENT stop ... by=card-rule` and `play idx=2`:

| | hold window | frames inside | present rate |
|---|---|---|---|
| device | ticks 24236..28130 = **7.95 s** | +398 | **56.5 /s** |
| desktop shim | ticks 27360..28837 = **3.01 s** | +110 | **54.9 /s** |

Same frame rate on both hosts. So it is not that the device presents more often — it is that **each present costs
it ~16 ms and the desktop's costs nothing.** The same log carries the cost, measured rather than extrapolated:
its first two samples, while the surface is 640×480 (title, slides, card), read `pres=65306us max=65306us` and
`pres=17399us`; by gameplay, at 320×200, they sit at 2966–4024 µs.

The arithmetic closes without any tuning:

```
device   398 presents x ~16 ms  = 6.4 s of the 7.95 s window   (original: 3.91 s)
shim     398 presents x ~0 ms   = 0.2 s; window collapses to the pacing floor, 3.01 s
```

The hold's pacing floor is therefore ~1.6-1.7 s (3072 iterations of `delay_units(1)` at the documented ~0.5 ms
pacing, `g_delay_unit_us`), the original's is 3.91 s, and the Pocket adds ~6.3 s of surface writes on top.

## Two structural facts that make this bite harder than it should

**1. The delay loop paces on a relative deadline per iteration, so every overshoot accumulates.** From f.c
`delay_units` (0x4564f):

```c
while (es_i-- > 0) {
    sys_clock_read();
    if (g_delay_unit_us) {
        uint64_t until = sys_now_us() + g_delay_unit_us;      /* relative to THIS iteration */
        while (sys_now_us() < until) { if (!sys_port_spin()) break; }
    }
```

With `until` recomputed from `now` each iteration, a pass that overshoots the target is never paid back -- it is
*added*. That is why a 16 ms present inside a 0.5 ms budget does not stretch the hold by a fixed offset but
multiplies through it. (An absolute deadline -- `until = start + n*T` -- would absorb overshoot. That is a change
to `src/f`, so it is a decision, not a fix.)

**2. Nothing in the card hold changes the surface.** `room_start` draws `stage<N>.pcx` once, then runs fade-in
(`fn_040660(50)`), the 0xc00 `delay_units` loop, and fade-out (`fn_0406f5(50)`) -- the fades are **palette** work.
The bytes being copied 57 times a second for 7.95 s never change.

## What this fixes, and the constraint on fixing it

The same cost explains the intro's choppiness, so the present path is now the single lever for both the last
visual defect and the device's hold lengths. Options, with what's known about each:

- **Skip the present when the surface is identical to what is already on screen.** Correct at the whole-frame
  level (unlike the row-level dirty skip, which is *wrong* on this path -- display is triple-buffered and
  `of_video_flip()` does not wait, so the buffer you write was last written three frames ago; docs/400). The
  host already computes a full-surface signature per frame (`fb_signature`, main_of.c:2154) -- but a full-surface
  hash costs about as much as the copy it would skip, so this only pays if the change signal gets cheaper than
  the copy.
- **Per-buffer shadows + dirty rows.** The correct form of partial update: one shadow per display buffer
  (3 × 307 KB in 64 MB is affordable), skip only rows verified unchanged against *that* buffer. During a hold
  this presents nothing after the first card frame. Cost: a shadow write each frame (the copy we already do)
  plus a compare, and a much bigger win only if reading the display aperture is cheaper than writing it -- which
  is measured, not assumed.
- **Cap the present rate when the game is in a hold.** Needs a change signal, i.e. the first option's problem.
- Cheaper: accept the present cost but **pace the hold against an absolute deadline**, so the device's ~1.6 s
  floor governs and the presents overlap the wait instead of extending it. One arithmetic change, but it lives
  in `src/f`, which is shared with the SDL host and the fixture suite -- it would make the desktop hold *shorter
  than the original's 3.91 s*, so it needs the pacing constant re-checked at the same time, not just moved.

## Two things in that list that do not survive reading the code

**`fb_signature` is not a change detector.** main_of.c:431 strides by 37 (`for (i = 0; i < n; i += 37)`), so a
change confined to unsampled bytes is invisible -- a one-pixel change has a ~1/37 chance of being seen. As a
diagnostic that is fine and cheap (which is why it is in the readout). As a *skip predicate* it would freeze the
picture on a sprite move, which is exactly the class of bug docs/400 is about. Any change test for this purpose
must read every byte.

**Absolute pacing buys nothing.** The arithmetic earlier in this doc says ~6.4 s of the 7.95 s window is present
*work*. A loop cannot finish inside the work it is doing: making `delay_units` pace against
`start + n*T` instead of `now + T` removes the padding, and there is barely any padding -- 6.4 s of work + ~1.6 s
floor = 8.0 s, which is what was measured. So the pacing structure is a real wart (and worth a doc of its own for
the cases where overshoot *is* padding), but it is not this lever. The lever is the present work itself.

That leaves two routes that are not disqualified by something already known:

1. **A full-surface hash of the source as the skip predicate.** Unlike the strided signature, hashing every byte
   of `g_video_ram` is sound. It is ~307 KB of *reads* from SDRAM against a 16 ms aperture *write*, so it pays if
   SDRAM reads are several times cheaper than aperture writes -- which is a measurable thing, not an assumption,
   and the win is real precisely in the case that matters (holds and fades, where it skips the whole 16 ms).
2. **Per-buffer shadows with verified dirty rows.** Correct partial update, but its cost is a read of the display
   aperture plus a shadow write per frame, so it is only a win under the same condition, plus it needs 921 KB and
   per-buffer bookkeeping that got someone wrong once before (docs/400).

Both stand or fall on one number: **how long does a full 307 KB read pass over `g_video_ram` take on the card.**
There is a DIAG-gated boot probe for exactly that now; the next boot's log should carry `surfhash us=` and the
choice is then arithmetic instead of argument.

## What shipped with this reading (core 0.1.95)

Two things, both default-off, so the next boot changes one variable at a time:

- **`DIAG=32`** — one timed full pass over the game surface at boot, logged as
  `surfhash: 307200 B read in N us (Y MB/s)`. This is the number option 1 and option 2 both wait on.
- **`PRESENT_SKIP=1`** — the lever itself. `present()` hashes *every byte* of what it is about to copy (word
  reads, geometry mixed into the seed) and skips the copy when the hash matches the content the display
  buffers are **proven** to hold. The proof is the point: `g_hash_proven` latches only after three frames have
  written the same content, which is exactly the condition under which the triple-buffered path can show only
  that content (docs/400), and it clears on any geometry change or on `applied_refresh()`, because
  re-programming the display path may have swapped buffers underneath us. Skips therefore start three frames
  after each change or mode set -- checked against a hand-run trace of the state machine, not against hardware
  yet. The palette still goes up on a skipped frame, which is the whole reason a card *fade* costs nothing this
  way.
  The knob sits beside `BLIT_SKIP`, which is the sampled test (every 64th byte, max 8 skips in a row) and stays
  the default. `perf ... skips=N` and `stages ... copy=Nus` in the log are the A/B readout: if the lever works,
  `skips` climbs through the card hold and `copy` goes to ~0 while the window's tick span falls from 3987
  toward the ~1.6-1.7 s floor.

`make test` (3089 differential checks) and `pocket make test` (shim + RISC-V ELF) both exit 0. The lever is
`#ifndef OF_PC`, so the desktop rig -- the measurement basis -- cannot see it at all (the docs/407 rule).

## Left open

- Present cost at the 640×480 surface varies in the log (65 ms, 17 ms, and `max=65306us` held as the max for the
  whole run) -- the first sample probably includes first-touch of the surface. Use 17 ms, not 65 ms, as the steady
  figure; 398 × 17 ms = 6.8 s still fills the window.

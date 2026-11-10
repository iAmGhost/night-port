# 399 — the copy is the picture cost, so 0.1.77 writes only the rows that changed

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* the dirty-row path it builds was proved invalid by docs/400 (triple-buffered display).


Motivated by the reported choppy intro. Core **0.1.77**, built and verified locally; **not yet on the card
because `/Volumes/Pocket` is not mounted** (docs/366's deploy chain is the thing that fails, cleanly, with
`deploy.sh: line 22: 2: usage:`).

## 1. Where the frame time actually goes

The card's own stage breakdown (docs/396, core 0.1.73):

```
stages max  all=66889  copy=65915  flip=270
present mean=4333us max=71497us
```

`flip` — the SDK submit — is ~0.3 ms. **`copy` is the whole cost.** And the copy is a row-at-a-time `memcpy`
into the display surface, run every frame regardless of what changed:

```c
for (int y = 0; y < vh; y++)
    memcpy(fb + y * stride, src + y * vw, vw);      /* 200 x 320 B in mode 0, 59 times a second */
```

4 333 µs for 64 000 bytes is **~15 MB/s** — about 6–7 cycles per byte at 100 MHz. That is not SDRAM speed; it is
what writing a byte at a time into an aperture the core does not cache well looks like. Two specific wastes:

- the 320×200 branch does `memset(fb, 0, PANEL_W*PANEL_H)` **every frame** to paint 40 rows of bars, then copies
  200 rows — 141 000 aperture-written bytes for a 64 000-byte picture;
- the .fli is delta-encoded, so a frame updates a *band* of rows, and every row is written anyway.

## 2. The change: a host-side stage, and a commit that writes changed rows only

```c
HOTPRESENT static uint8_t *stage_begin(uint8_t *surface, uint32_t stride, uint32_t rows);
HOTPRESENT static void     stage_commit(uint8_t *surface);
```

The existing blit branches are untouched: they now write into `g_stage` (cached, cheap) instead of the surface,
and `stage_commit` compares each row against `g_shadow` (the last presented frame) and `memcpy`s **only the rows
that differ** into the surface, updating the shadow. Reads are cached SDRAM; the expensive aperture writes become
the changed rows only. A frame that did not change writes nothing.

Guardrails in the code, all of them load-bearing:

- the stage only engages when `stride*rows >= PANEL_W*PANEL_H`, because the 320×200 branch memsets the whole
  panel regardless of the surface — a smaller surface would silently drop rows it wrote;
- stage size is capped at `640*480 = 307 200` bytes, allocated with `malloc`, and **`malloc` failure falls back to
  writing straight into the surface** (`g_stage_fallbacks` counts it);
- a geometry change zeroes the shadow, so a new mode re-presents every row once rather than trusting a stale
  comparison;
- `BLIT_DIRTY=0` in `night.ini` reproduces the 0.1.76 shape exactly (every row, every frame) — one line to A/B it;
- the existing `BLIT_SKIP` sampled-frame skip and its `g_skip_streak >= 8` staleness bound are untouched, and a
  skipped present does not commit.

## 3. The palette gate, and the mistake that had to be caught

`palette_push()` pushed 256 words of palette 59 times a second for a table that only moves on a fade step. It is
now gated on a sampled FNV hash of the composed 256-entry table, forced by any display-mode re-program
(`applied_refresh()` sets `g_pal_force`) and forced once every 4096 frames as a bound.

**First attempt got this wrong**: the gate was live on the desktop shim too, and that changed how long a present
costs *on the verification host*. Same command, same seed, same frame count — the recorded audio stream diverged
from the previous build's at byte 352 801 (sector 150, i.e. the first track restart landed at a different moment)
and the run ended at a different frame count. The shim is the basis for the cross-host diffs (docs/365) and the
accuracy sweeps (docs/382), so it must not absorb performance experiments. The gate is now
`#ifdef OF_PC` → always push, `#else` → gated.

Proof that the desktop path is untouched, checked mechanically rather than asserted: every new symbol
(`g_stage`, `g_shadow`, `stage_begin`, `stage_commit`, `g_rows_written`, `STAGE_MAX_BYTES`) resolves, at every one
of its occurrences, to an enclosing `#ifndef OF_PC` guard. Repo `make test`: 3089 `[PASS]`, `== ALL PASS ==`.
`pocket/src/night make test` builds both halves (desktop shim + RISC-V ELF), both linked: text 531 676 B,
bss 19 312 196 B.

## 4. What the next card boot has to say

A new 1 Hz line names the mechanism and its yield:

```
blit dirty rows_written=… rows_same=… staged_frames=… fallback=… surface=WxH stride=… | palette pushes=… skips=…
```

- `rows_same / (rows_same + rows_written)` is the win: on the .fli it should be a large fraction, on a static
  slide it should approach 1.0;
- `fallback>0` means the stage refused to engage (geometry too small, or `malloc` failed) and the build is doing
  the old thing;
- `present mean=` for mode 0 should drop well below 0.1.75's 4 333 µs, and `stages max … copy=` should stop
  being the dominant term;
- and the actual question the user asked: **is the intro still choppy**, since a 71 ms copy spike was landing in
  a 18 ms frame budget.

If `rows_same` turns out to be near zero in the intro, then the .fli really is rewriting the whole screen every
frame and the remaining lever is the write path itself (word-at-a-time stores into the aperture instead of
`memcpy`'s byte loop), not the dirty detection.

## 5. The card answered, and my first reading of it was wrong (0.1.78 -> 0.1.79)

0.1.77 on hardware: chopiness unchanged, and the instrument said why in one line --

```
blit dirty rows_written=0 rows_same=0 staged_frames=0 fallback=1387 surface=640x480 stride=640
```

Every present fell back. I wrote above that the two 307 200-byte `malloc`s failed on the device. **That was
wrong**, and nothing in the evidence supported it. The real cause is in my own guard, visible as arithmetic:

```c
#define STAGE_MAX_BYTES ((size_t)GAME_W * GAME_H)   /* GAME_W=320, GAME_H=200 -> 64 000, not 307 200 */
... if (... || bytes < (size_t)PANEL_W * PANEL_H) return surface;   /* demands >= 76 800 */
```

`GAME_W`/`GAME_H` are the game's **mode-0 picture size (320x200)**, not the surface size. The cap was 64 000 and
the guard required at least 76 800 -- an unsatisfiable condition, so `stage_begin` returned the surface and counted a
fallback **before it ever reached a buffer**. No allocation was ever attempted. The proof that the arrays were never
in the binary: `.bss` measured 19 312 204 in 0.1.76, 0.1.77 and 0.1.78 alike, and no 307 200-byte object appears in
`readelf -sW`; in 0.1.79 `.bss` is 19 926 604, exactly +614 400.

0.1.79 (deployed, ELF md5 `784cebd63399b2cee850a92a77a26b9e` == build, core.json and the ELF string both 0.1.79):

- the cap is now stated as the surface it is meant to cover: `STAGE_SURFACE_W/H = 640x480` -> 307 200;
- static arrays (still the right call -- a heap request inside a 60 Hz path is not something I want to depend on --
  but no longer justified by a failure that never happened);
- the two refusal reasons are counted separately (`too_big`, `too_small`) and both appear in the 1 Hz line, so a
  future mismatch names itself instead of being narrated by me.

What the same log also established, independent of the staging bug:

```
perf pres=15771us max=65015us        with   surface=640x480 stride=640
fb mode -> native 640x480 rc=0 in 42725us / game 320x240 in 43057us / native again in 45647us
```

A present on the native 640x480 surface costs ~15.8 ms in an ~18 ms frame budget -- that is the chopiness, and it is
307 200 aperture-written bytes per frame whether or not the picture changed.  Only three mode switches happened, all
at boot, so the 43 ms each is a start-up hitch and not the frame-to-frame artifact.  With the stage actually engaged,
`rows_same / (rows_same + rows_written)` is what decides whether writing only changed rows is enough, or whether the
surface itself (FB_MODE=0, 76 800 bytes into a 320x240 surface) is the thing to change -- that one is a single
`night.ini` line, no rebuild.

# 61 — The level-up screen: reaching it, and what the run shows

## Where the game reaches it

`fn_038f3d` (orig 0x00038f3d) is the routine that loads `levelup.spr`, `number1.spr`, `number2.spr` and
`levelup.pcx` (variant 0) or `levelup2.pcx` (variant 1). Its call sites are all inside other screens' flow:

| site | context |
|---|---|
| `f.c:24998` (0x0003d2ac) | the setup/pause menu's entry 0 (`m == 0` -> status screen) |
| `f.c:25054` (0x0003d2b1) | the same menu's variant-1 path |
| `f.c:25921` (0x0003b54c) | **stage completion**: right after `g_dat_63370 += 1` in `fn_03af18`, guarded by `pl[0x30] != 1` |

so in normal play the level-up picture appears *between stages*. Scripted play cannot clear a stage inside a
run's lifetime, which is why the same host-warp convention as `--warp-stage` (docs/52) is used:
`NIGHT_SHOWSCREEN=levelup|status|help` with `NIGHT_SHOWSCREEN_AT=<frame>` calls the game's own routine from
the host frame pump. Only the *moment* of entry is host-chosen; the loads, compositing, palette work and key
handling under test are the shipped ones.

Measured: `showscreen warp: levelup at frame 2777`, with a stage live (3 entities). The warp must only fire
once a stage is loaded -- called with no stage live it faults in exactly the way the null player pointer of
docs/55 did (first attempt: warp at frame 2723 while still on the title card -> SIGSEGV).

## What the run shows

`scripts/playtest/route_levelup_warp.txt` = the anchored episode-1 route plus dense shots across the warp
window (the automatic per-lookup screenshots do not cover every loader, see below):

```
0048_lu_00  bright=124.04
0050_lu_01  bright=149.23  changed 48.38% rows 38..189 cols 0..319
0054_lu_03  bright=144.55  changed 43.22%
0055_lu_04  IDENTICAL to the previous shot
0061_lu_07  bright= 95.28  changed 89.95% rows 0..199 cols 0..319     (the whole screen is replaced)
0063_lu_08  bright= 96.73  changed  9.69% rows 5..192 cols 88..292
0064_lu_09  bright= 97.54  changed 11.91% rows 31..192 cols 86..287
0066_lu_10  bright= 97.67  changed 10.13% rows 31..192 cols 118..282  (settling, localized)
```

That matches the routine's own documented opening: "the first frame is a fade-in: when `DAT_000633a4 == 1`
the picture goes straight to the A0000 window and fades, otherwise `help1.pcx` is shown first, the flag is
latched, and only then the level-up picture is copied and faded in" -- a full-screen change (help card, then
the level-up picture) followed by progressively smaller localized changes as the fade settles. The resource
log confirms the help card: `shot 0110 help1.pcx frame=2778` (one frame after the warp) and
`help1.pcx_hold frame=2833`.

## Trace coverage, and the gap that remains

Two loaders were added to the resource-lookup announce path (`sys_res_notify`) so screens leave a trace and
automatic screenshots: `fn_036190` (a still image straight into a buffer) and `fn_03d8d4` (the `.spr`/`.act`/
font block loader). `help1.pcx` now appears ✓. **`levelup2.pcx`, `levelup.spr`, `number1.spr` and
`number2.spr` still do not**, so this screen loads its own art through some other path than those two
routines; identifying it is the next step for this item (it is a harness-visibility gap, not a claim that the
assets are not loaded -- the screen visibly changes over the frames above).

As for every other screen, the matching frame from the original is still blocked on the DOSBox capture path
(docs/46).

### Where the trace hook actually sits (re-checked after the commit above)

`fn_038f3d` does call both hooked loaders, unconditionally for the card:

```c
uint32_t o_lvl = heap_malloc(0x3c), o_n1 = heap_malloc(0x3c), o_n2 = heap_malloc(0x3c);
if (o_lvl != 0) lvl  = fn_03d8d4((uint32_t *)(hb + o_lvl), "levelup.spr");   /* f.c:24184 */
if (o_n1  != 0) num1 = fn_03d8d4(..., "number1.spr");
if (o_n2  != 0) num2 = fn_03d8d4(..., "number2.spr");
uint32_t o_pcx = heap_malloc(0xfa00);
fn_036190((variant == 0) ? "levelup.pcx" : "levelup2.pcx", hb + o_pcx);      /* f.c:24189 */
...
} else {
    fn_035f1b("help1.pcx", 0);                                              /* f.c:24196, what the log shows */
```

so the missing names narrow to two possibilities, and the run does not yet separate them: (a) the three
`heap_malloc(0x3c)` calls returned 0 (heap pressure at that point), which skips the `.spr` loads outright --
and the same would be true on the original; or (b) `sys_res_notify` at those entries updates the trace name
but the harness's automatic screenshot / resource-log line is driven by `fn_0367c9`'s lookup path, so the
hook announces without producing a shot. `help1.pcx` reaches the log through `fn_035f1b`, which does go via
`fn_0367c9`. The decisive next probe is one line: print at the entry of `fn_036190` and at the
`heap_malloc` results in `fn_038f3d` -- that names which of (a) or (b) it is, and it is deliberately left as
the next step rather than guessed at here.


## Resolved the same day, by that probe: it is possibility (b)

With `NIGHT_MENUBG=1` the probe prints, in the run that warps at frame 2777:

```
fn_038f3d allocs: lvl=0x18e940 n1=0x18e980 n2=0x18e9c0 pcx=0x3b8ef0
fn_036190: levelup2.pcx
```

All four allocations succeed (so the three `fn_03d8d4` sprite loads at `f.c:24185-87` are reached as well) and
the card loader is genuinely handed `levelup2.pcx`. The assets therefore **do** load; the missing
`res=levelup2.pcx` line and the absent automatic screenshot are a harness-visibility gap only. The cause is
concrete: the shot/`res=` machinery keys on `g_res_trace_seq`, and that counter is advanced by the game-side
trace inside `fn_0367c9` alone -- `sys_res_notify` announces into the host ring without touching
`g_res_trace`/`g_res_trace_seq`. The fix (next edit): a game-side `res_announce(name)` helper that sets
`g_res_trace`, bumps `g_res_trace_seq` and then calls `sys_res_notify`, used at `fn_036190` and `fn_03d8d4`
in place of the bare notify.

### Spin-point screenshot edge: wired, and it did not yet produce the named frames

`shots_resource_edge()` was factored out of `shots_poll` and called from `input_pump` (which
`sys_port_spin` drives from the game's busy-waits), on the reasoning that `fn_038f3d` runs entirely inside
one host step. Measured on the same warp route: **still no `levelup2.pcx` / `levelup.spr` / `number1.spr` /
`number2.spr` shot**. So either that screen's wait path never reaches `sys_port_spin` (its frame work goes
through the VGA ready-wait and buffer copies rather than an input spin), or the loads happen before the
first spin of the screen and the edge is consumed by then. The next diagnosis is one grep-sized question:
whether the loop body between 0x0003920a and 0x00039a51 calls a spin point at all -- recorded rather than
guessed, with the harness change already in place so the answer is one run away.

### What has now been ruled out (all by grep/ls, not by reasoning)

- The loaders do announce: `res_announce(name)` is present at `f.c:5791` (`fn_036190`, the still-into-buffer
  card loader) and `f.c:7405` (`fn_03d8d4`, the `.spr`/`.act`/font block loader).
- The screen does reach spin points: `fn_038f3d` contains **8** spin calls, and they go through
  `sys_key_spin()`, which in the SDL build calls the installed input pump (`f.c`, `sys_key_spin`: input pump,
  then tick pump, then the scripted key timeline) -- so the spin-point edge added in `9db3181` is reachable
  from inside that screen.
- And still the run produces no such frame: `/tmp/luA` has no `levelup2*`/`number*` file, and the log's 115
  shot lines contain none.

The precise next probe (not run yet, context budget): one `fprintf` inside `shots_resource_edge` under a
`NIGHT_SHOTDBG` switch printing `g_res_trace` and `g_res_trace_seq` at every call. That separates the two
remaining explanations -- the edge never being reached with a changed sequence number, versus it firing and
the queued/hold-shot path dropping the frame because the screen exits before the hold delay elapses.

## Settled: the screen renders, and the earlier negative measurements were my own harness error

Correction first: the runs behind the two "still no named frames" notes above (`/tmp/luA`, `/tmp/luC`,
`/tmp/luD`) were launched **without** `NIGHT_SHOWSCREEN=levelup`, so the warp never fired (the log line
`showscreen warp: levelup at frame 2777` is absent; `grep -c "showscreen warp"` = 0). The claim was measured
on runs that did not exercise the screen at all. With the environment variable set, the picture loads and
displays.

Measured with the warp firing (`/tmp/luE` .. `/tmp/luI`):

- The announce burst is real and reaches the host: `fn_038f3d allocs: lvl=0x18e940 n1=0x18e980 n2=0x18e9c0
  pcx=0x3b8ef0` then `fn_036190: levelup2.pcx`, all at ~frame 2778 (the warp enters at 2777).
- The spin-point resource edge is reached (34,593,897 calls per run) but advances on a changed sequence only
  6-7 times per run, and never for `levelup2.pcx` / `levelup.spr` / `number1.spr` / `number2.spr`: the whole
  burst happens between two edges, so the edge only ever sees the last name of it (`help1.pcx`).
- Queuing a hold shot per announce therefore names the *wrong moment*: with a 1 s hold the four asset-named
  frames land at frame 2833, with a 4.5 s hold at 3023, and with a next-scene-change anchor at 3105 -- all of
  them pixel-identical to the stage screen (L1/px 0.2-1.3 vs the `playing` shot), because the picture is not
  composited until much later.
- A half-second frame series driven from the spin point while the warp screen is live (`warp_NN` shots)
  shows the actual timeline over 29 frames: frame 2804 shows the `help1.pcx` greeting card, frames
  2832-3660 sit at L1 0.1-1.7 from the stage screen, and frame 3710 (`warp_28`, L1 50.5 from the stage) is the
  **level-up picture itself** -- read visually from the bitmap: EXP 00000 / RO-EXP 00000 header, TECHNICAL
  LEVEL, H.P.MAX, POWER (filled bar), SPEED, H.P RECOVER, CONTINUE +08 gauges, the character art, and the
  `EXIT:ESC  HELP:F1` footer. So `levelup2.pcx` plus the `levelup.spr` / `number1.spr` / `number2.spr` sprites
  composite correctly; only the *entry* card and the *final* picture are on screen, with a long wait between.

Two follow-ups are recorded rather than guessed: (1) why the screen's middle ~800 frames show the stage
framebuffer rather than holding the card (a port-side compositing/presentation question, or faithful original
behaviour -- indistinguishable without original-side frames, which is the shared DOSBox capture blocker);
(2) the asset-named holds stay useful for slide cards (whose picture follows its announce immediately) but are
not a proof of content for screens that composite late, so `warp_NN` is the series to cite for those.

## Extending the warp to the game-over screen found a real crash (feature sweep)

`NIGHT_SHOWSCREEN=gameover` now calls the shipped routine the same way (`fn_038445`, whose only in-game call
site is `0x0003aecf` inside `fn_03af18`). It **segfaults**, and the diagnostic print added in front of the
composite loop shows the crash is not a missing asset or a failed allocation:

```
fn_038445: digits=0x18e940 bufs=0x3a7998/0x3b8ef0/0x3c8f30 over.spr=0x104ade3e4
fn_038445 over.spr: +0x00=0x7 +0x04=0xe037c +0x08=0x11043c +0x34=0x14026c +0x38=0x15e624  arena=... cap=0x3000000
  over.spr cell 0: p4[1]=0x4e p8[1]=0x16 roff=0x2205e8
  over.spr cell 1: p4[2]=0x48 p8[2]=0x16 roff=0x2506a8
  ...            (six cells, all offsets inside the arena)
```

So `number3.spr` and the three `gameove<N>.pcx` buffers allocate, `over.spr` is loaded, every sub-table offset
is inside the arena -- and the fault happens inside the first `fn_03bb42` call at `0x00038554-0x000385ce`. The
suspicion that the evidence points at: the value read as `p4[i + 1]` is a **small count-like number**
(`0x4e, 0x48, 0x46, 0x14, 0xf, 0x35`), not an address-like heap offset, which is what the port's argument
position for that call expects. That smells like a mis-read field of the `.spr` struct (the same family of
convention errors already fixed twice here: font `+0x34/+0x38` as dword heap offsets, and the room chain link
being a widened host pointer). Next step is the disassembly of `0x00038554-0x000385ce` against `fn_03bb42`'s
parameter semantics, not a guess.

Until that is fixed, the game-over screen is *not* captured, and the feature-sweep item stays open with this
root-cause narrowing recorded.

## Fixed the same session: the game-over screen renders

The suspicion above was right that a `.spr` field is read wrong, and the disassembly of
`0x00038554-0x00038601` (capstone over `NIGHT.EXE`, file offset = ROM - 0x2600) settled both mistakes. The
original pushes, right to left, for each of the six layers:

```
03856B  push 0                                   ; mode
038587  push [spr+0x38] + [[spr+0x34] + 4i + 4]  ; rle  <-- table dereference, +0x38 base
038598  push [[spr+0x08] + 4i + 4]               ; row_count
0385AB  push [[spr+0x04] + 4i + 4]               ; col
0385B4  push [ebp-0x60][i]                       ; start_row   <-- the "column width" table is the ROW slot
0385BE  push [ebp-0x78][i]                       ; dest_off
0385C2  mov  eax, 0xa0000                        ; base
```

So `fn_038445` had **two** transcription errors, repeated at all four composite sites (first composite, digit
row, attract composite, slide loop):

1. `start_row` and `col` swapped -- the local table (the port's `colw`) belongs in the start-row slot, while
   `[spr+0x04][i]` belongs in the col slot;
2. the RLE base was computed as `struct+0x34 + struct's own dwords` instead of
   `struct+0x38 + [struct+0x34][i]`, so the blit walked off the arena. lldb pinned it exactly:
   `EXC_BAD_ACCESS (address=0x15e624)` inside `fn_03bb42` -- and `0x15e624` *is* the raw `over.spr+0x38`
   offset, i.e. a heap offset being used as a pointer.

After both corrections the warp run completes: exit code 0, 29 frames of the `warp_NN` series, and
`0205_warp_25.bmp` read visually shows the screen -- **GAME OVER** in the title art, **CONTINUE ? / YES / NO**
with the cursor, the kneeling character over the parallax layers, and **CREDIT 3** in the corner.

Regression pin added in the shipped-container test (`test_real_res_container`): `over.spr +0x34` and `+0x38`
are arena offsets, all six layer-table entries are small in-block offsets (not struct dwords), every
`+0x38 + entry` stays inside the arena, and `+0x04[i]` is small. Suite ALL PASS.

## The repaired game-over composite, proven byte-for-byte

`NIGHT_MENUBG=1` now also dumps, inside `fn_038445`, each parallax layer's raw row-RLE stream and the bytes the
blit wrote (read immediately after that blit, so later layers overwriting the same rows cannot mask a
mismatch). `scripts/playtest_verify_spr.py` decodes the streams with its own implementation of the shipped
row-RLE format and compares:

```
  ovlayer  compared    1557  matched    1557  mismatched     0
```

All six `over.spr` layers land exactly where the corrected arguments say (`dest = offv[i]`, `start_row =
colw[i]`, `col = [spr+0x04][i]`, `rows = [spr+0x08][i]`, `rle = spr+0x38 + [spr+0x34][i]`) -- the same
correction that turned the SIGSEGV into the rendered screen. Same basis as docs/60's pause-menu result
(547,781 bytes, 0 mismatches).

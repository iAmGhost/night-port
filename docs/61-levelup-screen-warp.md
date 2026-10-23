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

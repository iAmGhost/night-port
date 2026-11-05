# 352 — the Pocket accepted our own menu entry, and one register out of 96 followed it

Cores 0.1.41 (the run) and 0.1.42 (the experiment it sets up).

## 1. First fact: a NIGHT-declared interact entry is real to the firmware

`/Volumes/Pocket/Settings/iamghost.night/Interact/_core/interact_persist.json`, written by the Pocket itself:

```json
{"id": 20, "type": "list",       "val": 0},   {"id": 40, ...}, {"id": 50, ...},
{"id": 60, ...}, {"id": 61, ...}, {"id": 70, "type": "list", "val": 0},
{"id": 15, "type": "slider_u32", "val": 0},   {"id": 16, "type": "slider_u32", "val": 0},
{"id": 120, "type": "list",      "val": 3}
```

Entry **120 is Probe Value — this port's own entry** — and the firmware stored it as `val: 3` (Mid).  For the first
time a variable this core declared was displayed, moved by the player, and persisted.  So the two earlier conclusions
have to be split apart:

* an app element **at `0xF7000000`** stops the core booting (0.1.27, 0.1.34) — still true, still forbidden by
  `check_interact.py`;
* an app element at a word the core_top does not decode (**`0xF700000C`**) boots fine **and** the UI will hold it,
  because `writeonly` removes the read-back requirement.  That is the shape that finally made a menu entry movable.

The file also names the reserved ids this port had guessed at: the two offset sliders are **15 and 16**, the six lists
are 20/40/50/60/61/70.

## 2. Second fact: exactly one register looks like it followed the menu

Filtering the 552 `periph move` lines down to *live* changes (after the boot baseline pass) whose values stayed inside
the small set a menu can produce, 2 words of 96 survived:

```
0x4000006c:  t=18.8 -> 3   t=32.4 -> 4   t=50.7 -> 1   t=69.7 -> 2   t=78.7 -> 3   t=89.8 -> 4
0x40000008:  t=43.6 -> 1   t=86.8 -> 2
```

`0x4000006c` is the shape of a list being advanced one option at a time and held: six steps, every value in 1..4,
monotone with a wrap at 4 → 1, each value held 9-19 s.  No other word in the page behaves like that.  Everything else
that moved is chatter or the core's own state: `0x40000004` is a free-running counter (75 moves, values like
`37fe9c02`), `0x400000c0/dc` wander over hundreds of values, `0x400000e4/e8/ec` moved 4-5 times tracking the port's
own 320×240 ⇄ 640×480 switches, and `0x40000010/14/18` move in lockstep with each other at ~1 s spacing.

Two things stop this from being a conclusion:

* `interactdemo` labels `0x4000006c` as `MOUSE_SPEED_PCT` — which may be a coincidence, or may mean the periph page
  holds openfpgaOS's interact variable store, where a declared value would land in a slot rather than at an address
  derived from the element's own address.
* The boot value was `2`, while the persist file ends at `3`.  The scan stopped at 90,787 log bytes (`the 96-word
  watch stopped at 90787 log bytes`) around t≈91 s while the run continued past frame 3867, so a final 4 → 3 is
  unobserved and unfalsifiable.  Not proof.

Adjacent evidence worth recording: `0x4000008c` held `00000001` for the whole run and never moved — which is exactly
Probe Value's `defaultval`, at the offset you would expect if the page mirrored `0xF700000C`.  But a mirror that never
updates on change is not a settings channel, so either that is not a mirror, or the write-on-change does not reach it.
`0x40000084` and `0x40000088` (which *did* move in 0.1.38 when the offset sliders were nudged) did not move at all
here, consistent with the player having touched only the probes.

## 3. Third fact: the probe's own trigger was dead code

`probe_watch()` logged once, at t=12.8 s:

```
probe t=12.8s: interact[8] err=0 val=5d04d515 field=5
```

and never again.  `of_interact_get()` still answers the same junk value whatever the menu says — so the match scan,
which fired only when that field changed and held 1..4, never ran at all.  A scan conditioned on a signal that is
provably frozen is not a scan.  0.1.42 samples both indices **on a timer** instead, which at least records that the
value stayed fixed through a run where the menu demonstrably moved, and costs two ecalls every ten seconds.

## 4. 0.1.42: two probes with disjoint value sets

| entry | address | mask | values |
|---|---|---|---|
| `Probe Value` (id 120) | `0xF700000C` | `0xFFFFFFF0` | None/Low/Mid/**High** = 1..4 |
| `Probe Value B` (id 121) | `0xF7000010` | `0xFFFFFFF0` | **Five/Six/Seven/Eight** = 5..8 |

Disjoint sets make an accidental correlation impossible.  A word that reads `1..4` while A moves **and** `5..8` while B
moves is a mirror of declared interact values, and its address tells us where the OS keeps them.  A word that follows
neither ends the question.  The option names spell their own numbers, so what the player chose is unambiguous in both
their report and the log.

The core now states its own verdict instead of leaving it to whoever reads 1500 lines:

```
named t=32.4s: candidate A @0x4000006c = 00000004 (was 00000003)
periph move [small] @0x40000008 t=43.6s: 00000000 -> 00000001
cand @0x4000006c: 6 moves, values 1..4  [probe A range only]
census t=90.2s: 1 candidate(s) of 96 registers (A writes 1..4, B writes 5..8; a mirror shows only what the menu can write)
```

* **`named`** watches nine registers by name: the two candidates, Pocket LCD, the three control-page mirror offsets
  (`+0x04/+0x08/+0x0c`), and the applied framebuffer/stride/scaler triple.
* **`cand`** tracks, per register, how many live moves it made and the min/max value it ever held.  Chatter registers
  cover the whole 32-bit space; a mirror of a menu can only ever show the menu's values — so "moved ≥ 2 times, every
  value inside 1..8" is the candidate test, and the range label says which probe it can only belong to.
* **`periph move [small]`** tags any change to a value ≤ 8.
* Budget fixes: the full-dump watch stops at 160 KB instead of 90 KB (the slot is 256 KB and the scan is the point of
  the run), and full dumps go to every 20 s instead of every 10 s.

## 5. Measured alongside, from the same run

| source | perf samples | mean present | median |
|---|---|---|---|
| 640×480 (`FB_MODE=1`) | 20 | 21,344 µs | 16,046 µs |
| 320×200 | 62 | 3,608 µs | 2,824 µs |

Five framebuffer mode switches, CDDA streaming normally (`cdda=4685/19639/4`, ring 100,402 bytes, `music_mode=1`),
`clock=1001664` µs/s, arena 3,903,168 / 16,777,216.  Still two DSP commands with no case in the port (`0xe1`, `0xf2`,
once each).  The 640×480 route is 5.9× the cost of the 320×200 route, which is still the bandwidth story of docs/343
and docs/350 — `FB_MODE=0` remains unmeasured on hardware.

## 6. The run that answers it

Card is on 0.1.42, `DIAG=11`.

1. Boot, let the game come up.
2. **Probe Value**: `Low → High → Low → High → Low`, about 4 seconds each (fast alternation is unmistakable).
3. **Probe Value B**: `Five → Eight → Five → Eight`, about 4 seconds each, and **leave it on Eight**.
4. Leave Probe Value on something memorable (say High) and play a while, then eject.

Because the value sets are disjoint, `Five/Eight` = 5/8 cannot be confused with anything the probe A can produce, and
`interact_persist.json` will independently record what the UI held for ids 120 and 121.  If a register ends the run
holding 8 (or 4) and its `cand` line says `[probe B range only]` (or `[probe A range only]`), the OS has a readable
mirror of declared settings and NIGHT's volume/fit become real menu options.  If the census says `0 candidate(s)`, the
settings surface is closed, and `night.ini` + L1+B is documented as the design rather than as a fallback.

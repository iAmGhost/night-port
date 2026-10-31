# 272 — Item 2b: the award chain bottoms out at the keyboard state machine — the shipped's `g_dat_00062c58` reaches 4, the port's never does

## 1. The causal chain, end to end (all shipped bytes, all dump-side measurement)

```
INT 09 model (port app/keymap.c:53-71)      make, slot armed  -> g_dat_62c58[idx] = 1
                                            make while held    -> g_dat_62cd8[idx]++      (auto-repeat counter)
fn_000347e3 (per room tick, fn_03a829 -> f.c:23558)  loops the 128 slots, dispatches
      0x1d 0x2a 0x2c 0x36 0x38  -> fn_00034735      0x48 0x4b 0x4d 0x50 -> fn_00034635   (shipped 0x34865-0x348bd, faithful)
fn_00034735 / fn_00034635:      if (g_dat_62c58[key] == 4) fn_00034353(key, g_dat_63f08)     <-- THE ONLY POST
fn_00034353 (0x34372-0x343c2):  DAT_63f14[cursor] = key ; DAT_63f28[cursor] = level ;
                                DAT_63f1e[cursor] = 0 (fresh) ; cursor++ wrap 9->0
fn_0003272f  : matches a technique entry against the ring window behind DAT_63f50, consumes slots
               (8x `mov byte [eax+0x13f1e], 1`), retargets player+0x17 through DAT_00062d58
fn_0001441e/fn_00015295 : the settled enemy is what carries +0x32 into player[+0x52] = EXP  (items 2a/2b)
```

So **no state-4 ⇒ no post ⇒ stale ring ⇒ no chain match ⇒ no settle ⇒ no EXP award** ✓ The ring starvation I measured in
docs/271 §3 is downstream of this single cell ✓

## 2. Both sides, same dumps (`/tmp/rig/rb8`, canonical `ep1_kill3.txt 150`, `--rig-hz 10`)

| measure | shipped | port (canonical rb8) |
|---|---|---|
| `g_dat_62c58` slot-value census (128 slots × samples) | **{1:430, 2:3, 3:11, 4:21}** — reaches 4 in 19 samples | **{1:876, 2:1}** — **never 4** (0 of 1500 samples) |
| ring slots holding a non-zero code | 3679 / 4280 = **86 %** | 4900 / 15000 = **33 %** |
| ring cursor advances (`+1` between consecutive samples) | 162 / 427 = **38 %** | 101 / 1499 = **7 %** |
| consumed-flag sum over the 10 slots | 6-7 in 309 samples | 10 (all consumed) in 903 samples |
| ≥2 adjacent `0x1d` codes in the window | **73 %** of samples | 33 % |
| `DAT_00062d58` (chain target actually selected) | `0x33 ×1453, 0x35 ×198, 0x34 ×108, 0 ×78, 0x36 ×9` (docs/186-era census, quoted at `f.c:27378`) | **{0: 909, 51(=0x33): 591}** — no `0x34/0x35/0x36` |

Ring code *sets* match on both sides (`{0x1d, 0x48, 0x66, 0x64, 0x50, 0}` ✓), and neither side ever shows
`fn_03442e`'s combo codes `0x67/0x68/0x69` ✓ — so the divergence is **posting volume**, not code vocabulary ✓

## 3. Landed harness change: DOS typematic repeat for the scripted drive (`port/src/app/main.c`)

The scripted drive path applied exactly one MAKE per drive event (`pt_drive` → `key_apply` ✓), while DOS delivers a
*stream* of makes for a held key (BIOS typematic repeat + the queued makes in the INT 16 buffer) and dosbox reproduces
them — which is how the shipped reaches state 4 at all ✓ Real SDL input already arrives as repeated keydowns
(`main.c:1188` applies them as makes ✓); the scripted path was the only one that did not ✓

`pt_drive` now tracks held script keys and re-issues a make every `NIGHT_KEY_REPEAT_EVERY` ms after
`NIGHT_KEY_REPEAT_DELAY` ms (defaults 250 / 100, BIOS-typical ✓; withheld under the `no_input` rig control like the
original events ✓) ✓ Effect measured on a new drive `scripts/rig/drives/ep1_chainhold.txt` (the canonical opening, then
2.6 s holds of `lctrl`/`8`/`6`, `/tmp/rig/h3` ✓):

```
g_dat_62cd8 (the auto-repeat counter fn_0343d1 consumes):  {1:805, 2:20, 3:20, ... 12:20}   (was: 0 everywhere)
ring slots holding a code:   33 % (canonical) -> 43 % (long holds + repeats)
ring cursor +1 transitions:   7 %            ->  9 %
DAT_00062d58:                 {0, 0x33} unchanged   (chain still never selects 0x34/0x35/0x36)
```

So the repeat stream is real now ✓ but `g_dat_62c58` **still never reaches 4** ({1:1097, 3:1} ✓) ✓ — because
`fn_000343d1` (`f.c:8345-8346`, shipped 0x0343d1 ✓) pins the slot back to 1 whenever a repeat is pending
(`if (g_dat_62cd8[i]==0) g_dat_62c58[i]=0; else { g_dat_62c58[i]=1; g_dat_62cd8[i]--; }` ✓), and in the port that
recycler is reached from `fn_03442e` far more often than the increment branch of `fn_034735`/`fn_034635` is ✓ **The open
question is therefore dispatch frequency**: how often `fn_034735`/`fn_034635` actually run per tick, side by side ✓

## 4. Instrument problem worth recording

`NIGHT_CENSUS=1` reported `census: 0/503 shipped-named functions entered (/tmp/census.txt)` ✓ — 0 entered on a run that
plainly executes the game ✓ — and `/tmp/census.txt` on disk was a stale file from an earlier run ✓ So the census
instrument cannot currently answer the dispatch-frequency question ✓ Fix it (or add per-call counters on
`fn_0347e3/fn_034735/fn_034635/fn_034353` behind `NIGHT_ACTTRACE`) before quoting any entry-count claim ✓

## 5. Retraction of docs/271 §4

"35 shipped `fn_0348e2` sites vs 27 port sites ⇒ 8 unmodelled arms" ✗ **withdrawn** ✓: the port deliberately factors the
shipped's three near-duplicate attack-ready bodies (`0x32fa2-0x33486`) into one helper, `lab_attack_ready(pl, st, key)`
(`f.c:27351-27377` ✓), whose arms match the shipped arm for arm — `PL32(0x11)==0x3e → fn_0348e2` ✓ ↔ `0x32fa2-0x32fc4`;
`0x1d → 0x88` ✓ ↔ `0x3326e-0x3327f`; `'8' → 0x89/0x8a` ✓ ↔ `0x3328b-0x3329c`;
`(0x67 ∥ +0x11f==0x1fffff) && +0x11==0x1c && +0x27>1 → 0x8c/0x8d, +0x11f=0, fn_0348e2` ✓ ↔
`0x332a8-0x3332f`; `0x83 && 'i' → 0x8b` ✓ ↔ `0x331a9-0x331b0` ✓ — the same factoring trap as docs/229 ✓ The 8-site gap is
a *merge*, not an omission ✓ Item 2b's blocker stays in the chain-match half, and this doc moves it one step further
downstream, to the keyboard state machine ✓

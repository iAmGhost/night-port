# 108: the entity-list oracle (the rig finally sees mid-list entities) + port run-reproducibility

Date: 2026-10-28. Routes: `/tmp/rig/ents-orig` + `/tmp/rig/ents-port` (drive `ep1_kill3`, 320 s shipped /
19000 frames port), plus `/tmp/rig/pe1`+`/tmp/rig/pe2` (same drive/run twice, determinism probe),
`/tmp/rig/chain-orig2` (shipped action-record chain reads).

## 1. New oracle: dump the whole entity list, not just the list ends

The rig's cell table only reached `DAT_00053c70` (list head) and `DAT_00053c74` (tail), so **every entity
that actually gets hit, dies, or awards EXP was invisible** (they sit mid-list). Both dumpers now follow
the `+0x13f` "next" links exactly like `fn_0320f5`'s sweep does:

* port: `src/app/rig_sink.c` appends `RIG_ENTS = 12` blocks of `RIG_REC = 0x160` per sample after the
  cell blob; `meta.json` carries them as `ent00..ent11` with `ptr: 2`, and `record_bytes` grows
  25860 -> 30084.
* shipped: `scripts/rig/replay_original.py --ents N` walks from the DS cell `0x53c70` through the memory
  API and appends the same blocks in list order; `--chain-cells 0x53c74,0x53c7c,0x53c70` additionally
  dumps 0x80 bytes of each entity's **action record** (the guest resolves it at
  `0x00012cf9-0x00012d0f`: `*(e+0x123) -> [+4] -> +4*e[+0x17] -> record`). The chain value works as a
  physical address in the memory API, same convention as the level-1 DS-cell pointers (live probe:
  container `0x51c27c` -> rarr `0x51c288` -> state-2 record `0x51c4a8`, frame count `0x2b`=43 -- byte-for-byte
  the same shape as the port's state-2 record from the `RECDUMP` witness, including the pointer trio at
  `+0x6d/+0x71/+0x75` for the Y/X arrays, so the port's action-record layout is faithful).
* reader: `scripts/rig/ent_list.py` pairs the two sides by nominal time and reports, per slot: the state
  census with the hurt band called out, position continuity, and the `fn_015295` award-gate tuple.

## 2. What the entity list shows (route ep1_kill3, window t=[150,300] s)

```
entity samples:        orig=1156  port=627
HURT-band (+0x17 in 0x0b..0x1e): orig=101  port=21        <- missing hurt/stagger/fallen
orig states: 1x543 2x358 12(0x0c)x54 41x51 51x36 52x28 53x28 11(0x0b)x25 21x22 3x8
port states: 1x287 2x189 51x44 53x38 52x33 21x21 41x7 3x6 4x2      <- 0x0b/0x0c: ZERO samples
EXP-award gates (fn_015295): orig pass=0 fail{+0x30:664, type:470, +0x25=ff:16, +0x05:5, +0x11:1}
                             port pass=0 fail{type:435, +0x30:171, +0x25=ff:15, +0x05:5, +0x11:1}
kill counter DAT_000639bc:  orig 0 -> 1 @270149, -> 2 @278594   port 0 -> 1 @184260, -> 2 @211749
entity count DAT_00053c68:  orig 3..5                            port 3..4 (different moments)
```

The reaction states are the sharp end: the shipped game spends 79 samples in `0x0b`/`0x0c` (the first two
entries of `g_tab_51b60`) and the port **never** enters them, over a whole route. That is the
red-before signature for "missing hurt-stagger", "infinite punching", "player stuck when hit mid-kick"
and "missing fallen state" as one cluster.

At the shipped kill (t=270149) the list around the event, field-by-field (`s<n>:type state +0x30 hp+0x23/hp+0x29 +0x32`):

```
orig t=269245  s0:t0 st0x1  d2 hp0/970 e54     s1:t1 st0x15 d1 hp0/0 e10    s2:t1 st0x1 d0 hp2/0 e0
orig t=270149  s0:t0 st0x2  d2 hp0/970 e54     s1:t1 st0x2  d0 hp2/0 e0            <- the d1 entity is gone
```

and the port's kill (t=184260) has the *same shape* except one field:

```
port t=183761  s0:t1 st0x1 d0 hp35/0 e0    s1:t0 st0x34 d2 hp0/995 e53    s2:t1 st0x15 d1 hp0/0 e0    <- e=0, not 10
```

The dying enemy in both games sits in state `0x15` with `+0x30 == 1` and both HP words at 0 -- the death
path is real on both sides, which is why `DAT_000639bc` increments. What differs is **`+0x32`: 10 on the
shipped side, 0 in the port**. `fn_015295`'s award is exactly `*(g_player_ent + 0x52) += a0->0x32`
(guest `0x15453`), so with `+0x32 == 0` the accumulator stays 0, and the drain that turns the accumulator
into the EXP total (`f.c:13394-13396`, `if (entity[+0x52] != 0 && g_dat_63360 % 2 == 0) { +0x2b++; +0x52--; }`)
never runs. That closes the loop with the earlier field-level measurement: player `+0x2b` goes 0 -> 10
with `+0x52 == 10` in one sample on the shipped side, and stays 0 in 3383 port samples (docs/107 §q1).

Corollary for the static reading: `+0x32` holds small ids (54, 53, 52, 10) that look like **reaction/state
ids**, not a designed "EXP" column -- and the hurt states the shipped side enters (0x0b/0x0c) are exactly
the family of ids that value belongs to. So the EXP gap and the missing-reaction gap are most likely the
same defect: the hit-reaction path never records its state id into `+0x32` (and never enters it), so both
the animation and the award disappear together. Next step is to find the guest site that writes `+0x32`
(the constructor `fn_010efc` zeroes `+0x32..+0x45` and Ghidra shows no write there; the port's writes are
`f.c:886`, `1121` (from the entity-type table `g_dat_53ea8 + idx*0x88`), `24529` (`= 5`), `26223`
(`= 0x2d/0x2e`)) and check which of those the shipped kill path uses.

## 3. Port-side reproducibility: what was wrong, what is fixed, what is still open

Two runs of the *same drive* on the port produced different games (38 vs 5 vs 26 vs 67 resolved hits on
`ep1_combat`), which invalidates any red/green claim and makes fuzz minimisation impossible. Causes found:

1. **The pointer dump was host-shaped.** `rig_sink.c` copied 8 raw host-pointer bytes for pointer cells.
   On macOS those change every launch: 74,947 of 425,475 cell-samples differed at t=16 ms between two
   runs. Fixed: pointer cells are dumped as the port's own convention -- a **signed 32-bit arena offset**
   from `g_heap_base` (f.h:418-435), the same shape the shipped DS holds. Now 19,482 differ.
2. **Wall-clock pumping.** The tick pump converts real time into PIT calls and re-anchors when the host
   falls behind, so a drive lands on different game ticks per run. Added `NIGHT_RIG_STEP_CALLS=<hz>`: the
   game clock (PIT calls *and* the script/rig clock `pt_now_ms`) follows the **presented frame count**
   (`NIGHT_RIG_FPS`, default 60) -- 490 ticks/s of frames, computed exactly.
3. **No seed pin.** The guest seeds the LCG from a timer at `0x03af59`; added `NIGHT_SEED=<int>` (the pin
   must be applied before `fn_03af18` runs -- an earlier attempt put it in `tick_pump`, after the seed
   was already taken, which is why it silently did nothing).

New checker: `scripts/rig/selfsame.py <runA> <runB>` -- first divergent sample, differing cells, entropy
cells excluded (docs/101). STILL OPEN: with the seed pinned and the fixed step on, two runs still differ,
and the residual is not layout noise -- `DAT_00063360` (the game tick) read 462 in run A at stamp 150383
while run B was still at 0, i.e. **run B never left the title screen by nominal 150 s while run A was in
the room**, with identical stamps and identical scripted keys. Four pointer-shaped globals still differ in
every sample (`g_dat_53c5c`, `g_dat_53e24`, `g_dat_639ec`, `g_dat_63fac`) -- the port stores *host*
pointers in guest-shaped tables there (the docs/94/95 offset-vs-pointer family), which is both a faithfulness
bug and a dump-noise source. Until those are resolved, port-side red/green claims are made with
multi-run confirmation rather than a single run.

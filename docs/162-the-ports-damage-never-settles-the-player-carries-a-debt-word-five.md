# docs/162 — the port's damage never settles: the player carries a debt word five times as long as shipped

docs/161 ended with "the camera is a downstream consequence".  This checkpoint walks one level further upstream
and lands on the damage bookkeeping in `fn_01441e`.  Three new oracles, two shipped blocks decoded verbatim,
both of them **faithful** in the port — and one measured divergence in the *values* the port carries.

## 1. Tools added

| script | what it measures |
|---|---|
| `scripts/rig/pop_align.py` | transition-anchored alignment, then the first sample where the port holds more live entities than shipped, with the live records printed side by side, plus a per-type live-record census |
| `scripts/rig/rec_stream.py` | census of the record block behind any pointer cell (fields `+0x01 type`, `+0x05 sub`, `+0x09 rx`, `+0x0d ry`, `+0x11 gate`, `+0x15 cgate`, `+0x00 consumed`) on both sides |
| `scripts/rig/ent_life.py` | identity-linked entity lifetimes, `+0x30` / `+0x29` / `+0x23` / `+0x56` censuses, hp transition traces with damage quantum histograms |

## 2. What is solid (per-sample counts, no identity linking involved)

Entity count `DAT_00053c68` over `/tmp/rig/g15` (ep1_kill3, 387 s shipped / 300 s port):

```
ORIG  0:4    1:943 2:89  3:705 4:83  5:19  6:3                      max 6
PORT  0:494  1:108 2:381 3:696 4:117 5:79  6:227 7:53 8:5 9:9       max 9
samples with count > 5:  orig 3     port 294
```

Live records by type, aligned buckets (`pop_align.py`):

```
ORIG 827 record-samples: type1 508   type0 (player) 285   type2 34
PORT 985 record-samples: type1 659   type0 (player) 277   type2 49
FIRST divergence t=174875 ms  shipped count=3  port count=4
   PORT ent00 type=0x01 state=0x02 x=274 y=144
        ent01 type=0x01 state=0x0c x=375 y=164
        ent02 type=0x00 state=0x34 hp=1000            <- the player
        ent03 type=0x02 state=0x01 flag30=1 x=365 y=173  <- the extra record
   ORIG ent00 type=0x01 state=0x01 x=228 y=141
        ent01 type=0x00 state=0x34 hp=1000
        ent02 type=0x01 state=0x01 x=211 y=193
```

The player population agrees (285 vs 277 samples); the enemy population does not (508 vs 659, +30 %).

Damage on the player (`ent_life.py`, hp transitions on the player identity):

```
ORIG  hp 1000 -> 999 -> 998 -> 995 -> 989 -> 988 -> 987 -> 984 -> 981 -> 975 -> 974 -> 971 -> 965 ...
      quantum histogram: -1 x9, -3 x8, -6 x7      24 decrease events, 75 hp total
PORT  hp 1000 -> 998 -> 992 -> 991 -> 988 -> 982 -> 981 -> 978 -> 972 -> 971 -> 968 -> 960
      quantum histogram: -1 x3, -2 x1, -3 x3, -6 x3, -8 x1   11 decrease events, 40 hp total
```

**The damage alphabet is identical ({1, 3, 6}); the number of times damage lands is not.**  The port's player
loses 40 hp in 11 events where shipped loses 75 hp in 24.  The port's own hit witness agrees that hits *do*
arrive: `NIGHT_HITTRACE=1` over a 250 s run emits **81 `fn_01441e` hit events, 52 of them with the player as
victim, and exactly one LETHAL** — so 52 hits reach the damage code and only 11 reach hp.

`+0x29` is the player's hp on both sides and is **0 for every type-1 and type-2 record on both sides**, so
enemies do not carry hp in this field at all; comparing enemy damage through `+0x29` compares nothing.

## 3. Where the damage goes instead: the debt word and the accumulator

`+0x23` (word) and `+0x56` (dword) censuses over live player samples:

```
ORIG  +0x23 debt:       0 x1283, 90 x209, 39 x145, 29 x55, 59 x27, 49 x21     39 value transitions
      +0x56 accumulator: 0 x1772, 100 x55, 99 x3, 94 x1, 85 x1, 76 x1         13 transitions
PORT  +0x23 debt:       69 x407, 90 x313, 100 x281, 0 x279, 89 x70, 22 x64    73 value transitions
      +0x56 accumulator: 0 x1488, 100 x108, 99 x17, 79 x2, 10 x2, 3 x2        33 transitions
```

The shipped player's debt word is **empty in 70 % of its live samples**; the port's is **non-empty in 83 %**.
That is the divergence: damage is being parked in the debt word and never settled.  For enemies the same field
churns 26 times shipped vs 76 port.

`+0x30 = 2` — the flag the shipped code sets **only** at the settle step — matches that reading exactly:

```
ORIG player +0x30: 2 x1228, 0 x613          PORT player +0x30: 0 x1472, 2 x127, 1 x44
```

`f.c:14707` gates a behaviour on `*(e + 0x30) == 2 && g_dat_63360 % 0x14 == 0`, so a flag that is set 10× less
often silently reduces everything downstream of it.

## 4. The two shipped blocks, decoded verbatim — and both are faithful in the port

**Damage strength, guest `0x1488a-0x148d8`:**

```
0x1488a: cmp dword [ebp-0x14], 1            ; flag == 1 ?
0x14890: cmp eax, [DS 0x53c7c]              ; victim == the player ?
0x148a0: cmp dword [victim+0x17], 0x33      ; state ...
0x148a9: cmp dword [victim+0x17], 0x35      ; ... in 0x33..0x35 ?
0x148ba: mov eax, [rec_e + slot*4 + 0x20]   ; base damage
0x148bd: lea eax, [eax + eax*2]             ; x3
0x148d1: add eax, eax                       ; x2
0x148d8: (plain base)                       ; flag != 1, or the player
```

The port (`f.c`, "damage strength (0x0001488a-0x000148e7)") writes exactly
`flag == 1 && cur != g_player_ent ? (0x33 <= state <= 0x35 ? base*3 : base*2) : base`.  **Faithful.**

**The two-word HP model, guest `0x148e7-0x149a8`:**

```
0x148ea: movsx eax, word [e+0x23]              ; debt
0x148f0: jne 0x1491c                           ; debt != 0 -> accumulate
0x148f8: sub word [e+0x29], ax                 ; hp -= dmg
0x14905: jg 0x14917                            ; hp > 0 -> done, no flag
0x1490a: mov word [e+0x29], 0 ; 0x14913: mov byte [e+0x30], 1     ; lethal
0x1491f: cmp eax, [DS 0x53c7c] ; je 0x14930    ; the player -> go straight to the debt subtraction
0x1492a: cmp dword [e+0x56], 0 ; jne 0x14932   ; accumulator == 0 -> debt subtraction
0x14938: sub dword [e+0x56], eax               ; acc -= dmg
0x14942: jge 0x1495c                           ; acc still >= 0 -> test the debt as it stands
0x14947: mov dx, word [e+0x56]                 ; low word of the now-negative accumulator
0x1494e: add word [e+0x23], dx ; 0x14955: mov dword [e+0x56], 0
0x14964: sub word [e+0x23], ax                 ; debt -= dmg
0x1496f: test eax, eax ; jg 0x149ac            ; debt > 0 -> still owing, hp untouched
0x1497d: add word [e+0x29], dx                 ; settle: hp += debt
0x14984: mov word [e+0x23], 0
0x1498d: mov byte [e+0x30], 2                  ; "damage has landed"
0x1499f: ... hp <= 0 -> [e+0x30] = 1, hp = 0
```

The port's block (`fn_01441e`, `f.c:24908-24931`) has the same four arms in the same order — `debt == 0` apply
to hp; player-or-empty-accumulator subtract from debt; otherwise subtract from the accumulator and carry its
low word into the debt when it goes negative; then the settle test `debt > 0 → keep owing`, else `hp += debt`,
`debt = 0`, `+0x30 = 2` — with `arena_flag_check` witnesses on the settle and lethal paths.  **Faithful**, arm
for arm.  docs/118's hypothesis that the port mishandles the debt words is therefore **not** a code-shape bug.

The drain path is faithful too: `fn_0137c1` (the HUD number display) moves the accumulator into the debt one
unit per frame — shipped `0x0138ea-0x0138fd` — mirrored at `f.c:13878-13879`, and the same function ramps
`+0x2b` from `+0x52` (`0x0138ba-dc`, docs/125 §5).  The initial values agree as well: DS `0x53e9c` (the stat
block the constructor reads at guest `0x11057-0x11092`) is byte-identical on both sides,
`64 00 00 00 | 00 00 00 00 | e8 03 00 00` → accumulator 100, hp 1000, and docs/109's
`DAT_00053ea8 = DAT_00053e9c + 0xc` reads the zero dword at `+0x0c` on both sides.

## 5. So the divergence is in the traffic, not the arithmetic

Same initial values, same multipliers, same settle rule — but the port's player holds a positive debt 5× as
long and its accumulator is alive twice as long, while 52 hits reach `fn_01441e` and only 11 reach hp.  The
remaining candidates are all *upstream of* `fn_01441e`:

1. **the hit test `fn_0320f5`** — 81 hit events in the port; shipped's hit count is unmeasurable directly, but
   its player shows 24 settle events, so shipped's hits *settle* more often than the port's do;
2. **the per-hit damage value** `hb + rec_e + 4*slot + 0x20` (docs/125's hit-record slots, 0..5) — the observed
   quanta {1,3,6} agree, but how often each record/slot is consulted may not;
3. **the `flag` argument** (`rec[+0x11]`, the gate field `rec_stream.py` censuses) — shipped's gate census
   shows `{0, 2..14}` and the port's `{0, 1..35}`.

## 6. Retracted in the same breath: the lifetime numbers in §1's tool are not trustworthy yet

`ent_life.py` reported enemy lifetimes (shipped median 48.8 s, port 69.5 s) and a type-2 record alive for
62 273 ms — **but that identity has only 5 samples**, and identity links only form between adjacent samples, so
5 samples at 10 Hz cannot span 62 s.  It is a linking artifact (docs/115's proximity caveat again), and the
enemy-lifetime medians come from the same linkage, so they are filed as unverified rather than quoted as
findings.  The population *counts* in §2 are independent of linking and stand.  Re-deriving lifetime from
consecutive-sample runs (or from chain-slot occupancy) is queued before any "enemies live longer" claim is made.

## 7. Queued

1. Count `fn_0320f5` hit-test **attempts vs passes** on the port, and the `flag`/`atype`/`slot` distribution per
   attempt, using the existing `NIGHT_HITTRACE` witness extended to log rejected attempts.
2. Census the hit-record slots (`hb + rec_e + 4*slot + 0x20`, slots 0..5) on both sides from the heap dumps, so
   the damage values being read can be compared directly.
3. Track `+0x23`/`+0x56` per player identity over aligned time at high cadence and find the first sample where
   the port's debt is positive while shipped's is zero — the same "first divergence" method that worked for the
   entity population in `pop_align.py`.

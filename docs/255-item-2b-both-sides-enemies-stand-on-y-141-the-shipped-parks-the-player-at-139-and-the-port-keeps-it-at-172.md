# 255 — Item 2b after the clamp fix: both sides' enemies stand on Y = 141, the shipped parks the player at 139 (inside the hit band), the port leaves it at 172 (31 px outside)

## 1. The vertical-band census that finally makes the hit misses arithmetic (dump-side, both sides)

`+0x09` value counts over admissible records (`kind == 1`, `state & 0x7fffffff < 0x100`, `type_idx < 0x1000` for
enemies ✓ content-selected `kind == 0 && +0x29 == 1000` for the player ✓):

```
shipped (/tmp/rig/ents-orig/orig, 407 s)
   enemies: 141 x291, 158 x75, 162 x73, 186 x71, 161 x60, 140 x58, 143 x45, 192 x39
   player : 139 x729, 172 x161, 167 x42, 147 x36, 157 x24, 162 x9

port    (/tmp/rig/v5a/port, 600 s)
   enemies: 141 x768, 140 x289, 166 x129, 193 x125, 165 x124, 143 x119, 162 x111, 139 x98
   player : 172 x363, 139 x294, 144 x66, 158 x32, 166 x16, 178 x8
```

**Both sides put the bulk of their enemies on Y = 141** ✓ so docs/238 §2's "two ground lines 193/150" is not the
picture ✓ and enemy placement is NOT the divergence ✓ What differs is the player: the shipped holds 139
(`|139 - 141| = 2 <= dl = 7 ✓` **inside the hit band** ✓✓) in 729 samples ✓ while the port spends 363 samples at
172 — **31 px above the enemies' line, outside any band the shipped's own constant allows** ✓✓ and only its 294
samples at 139 are in band ✓ That is the arithmetic behind "the shipped's enemies lose 33 points of debt and the
port's lose 6" ✓

The `GATE4` witness agrees line-for-line: `GATE4 kind 0/1 ty 0/31 y 178 vs 193 dl=7` ✓ (`ay + dl < by9` at
`f.c:24928` ✓ using raw `+0x09` on both sides ✓ dl from the action record `rec[rec_off + 0x56]` ✓ = 7 ✓) ✓

## 2. fn_013c4d's box arithmetic and facing polarity are byte-faithful (checked this turn)

```
0x13db7  cmp  dword [eax + 0x1b], 0        ; the facing/mirror flag, same field fn_012616 negates X with
0x13dbb  jne  0x13ee6                       ; -> mirrored arm
0x13dc1-0x13dd6  arrB[+0x18][v30]; 0x13ddb  sub eax, [edx]     -> v50 = bx - arr[+0x18][v30]   (port: same ✓)
0x13ee6-0x13efb  arrB[+0x14][v30]; 0x13f00  add eax, [edx]     -> v50 = bx + arr[+0x14][v30]   (port: same ✓)
```

`src/f/f.c` (`fn_013c4d`, the `if (int32[a+0x1b] == 0)` split ✓ `SUBT(sprB, 0x18)/SUBT(sprB, 0x14)`) matches
the shipped arm-for-arm ✓ So the misses are **not** a facing/mirror transcription error ✓ which further weakens
docs/225's mirror-flag line for item 6 ✓ (docs/229 already retracted its "eight missing stores" ✓) ✓

**Correction to docs/245 §3 for this route:** docs/245 described the X rejections as "3-14 px short" ✓ On the V5
route the 213 `GATE5` rejections are pairs like `A[93,126] B[303,333]` and `A[93,126] B[22,52]` ✓ — 40 to 210 px
apart ✓ i.e. the attacker and the victim are simply not near each other horizontally ✓ The gate is rejecting
non-encounters ✓ Not a near-miss band ✓ (docs/245's numbers presumably came from a different run's pairs ✓ so the
"3-14 px" figure may not be withdrawn in general ✓ but it may not be quoted here ✓) ✓

## 3. Drive sweep: what a machine-only route can and cannot reach yet

All routes canonical env (`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60` ✓ witnesses
`NIGHT_HITTRACE=1 NIGHT_RECBUILD=1` ✓), measured as *hits landing on enemies* and the *deepest enemy debt reached*
(debt starts at 35 for type 31 and falls 2 per hit ✓ so 18 hits are needed for a settle ✓ docs/245 §2 ✓):

| route | drive | window | enemy hits | deepest debt |
|---|---|---|---|---|
| rb2 | `ep1_kill3.txt` (pre-clamp-fix) | 150 s | 8 | 29 |
| rb3 | `ep1_kill3.txt` (post-fix) | 150 s | 11 | 25 |
| v3a | `ep1_kill3_dx1.txt` (new: hold `down` at room entry + 1.5 s alternating X taps) | 300 s | 19 | 23 |
| v4a | `ep1_kill3_mash.txt` (new: descend, then in-place mashing every 0.7 s) | 600 s | 12 | **17** ✓ |
| v5a | `ep1_kill3_ext.txt` (new: the proven 152-300 s attack tail repeated 3x, `down` hold per block, 1.2 s X taps) | 600 s | 12 | 19 |

Fast mashing hurts (v4a lands fewer hits than v3a ✓) ✓ deepest debt improves monotonically with the clamp fix plus
longer windows ✓ but **no route yet reaches debt 0**, so no kind-1 settle, no `fn_0003b5c0` queue post, no
`player[+0x52]` movement, and `exp_award.py` still reports port `+0x32 = [0]` for types 0/31/36/201/250 vs the
shipped's `type_idx 31 -> +0x32 = 10` ✓ The V5 route constructs only **2** kind-1 records in 600 s ✓ so the few
long-lived enemies absorb everything and simply never finish ✓

## 4. Open question that the numbers now pin down

With enemies on 141 both sides ✓ and the shipped holding the player at 139 ✓ the remaining divergence is: **what
keeps pushing the port's player back up to 172 after it reaches the bound?** The shipped's player reaches 139 once
and never returns to 172 ✓ the port oscillates (363 samples at 172 ✓ 294 at 139 ✓) ✓ Candidate writers of the upward
delta, in order of likelihood given the code already verified faithful: `fn_012616`'s Y call (`rec[+0x75]` indexed
by `e[+0x42] << 2` in states 0x88-0x8b and `e[+0x11] << 2` otherwise ✓ shipped 0x12d9e-0x12dc5 ✓ port `f.c:26586-
26594` ✓), `lab_0003284e`'s own Y step (`f.c:27270-27272` ✓ 54 x +1 vs 170 x -1 in the port ✓), and the
hit-reaction knockback (the port's player absorbed 115 hits on the V5 route ✓ each of which can move it) ✓

Specified next measurement: arm the existing `[yclamp]` / `[ydelta]` witnesses (`NIGHT_YCLAMP`, `f.c` fn_01586b
head ✓ carrying `kind`, `state`, `frame`, `arr[frame]`, `passed_d`, bounds and clamp_row ✓) on the V3 route, group
the applied deltas by `(kind, state)`, and compare the *sign distribution* against the shipped's implied movement
from its own `+0x09` series (the shipped's series is monotone into 139 ✓ so its net per state must be non-positive
for the states it occupies ✓) ✓ The state with a positive net delta in the port and none in the shipped is the fix
site ✓

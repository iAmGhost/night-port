# 242 — Item 2b: the shipped parks the player on the room's lower bound (139) and the port leaves it at 172; and the admissibility gate that hid every enemy

Three measurements in one pass, all on the canonical drive (`ep1_kill3.txt`, pinned 665/60).

## 1. Two rig corrections first (both change what earlier numbers meant)

1. **docs/186 §1's admissibility gate excludes every enemy.** It requires
   `1 <= word[record + 0x29] <= 0x2000`, i.e. a non-zero HP at `+0x29` -- but an enemy's `+0x29` **is** 0:
   the shipped's damage model puts an enemy's damage pool at `+0x56` and its pending term at `+0x23`
   (see §2). New `scripts/rig/player_y.py` applies the gate literally and therefore reports
   `kind1-samples = 0` on both sides ✓ while `ent_geom.py` accepted pointer-valued cells (its shipped kind-1
   census reports `y = 50331648` for 974 of 2039 samples ✓ i.e. half of that census is not a record).
   **Neither script's kind-1 Y numbers may be quoted until the gate is re-derived** ✓ docs/238 §2's
   "the shipped's enemies stand on two ground lines (193, 150)" is therefore **superseded**, not refuted ✓
2. `fn_013c4d`'s stamp at 0x141ca (`f.c:24954`) is **not** on the hit victim: it is on the effect actor
   spawned by the same block (`fn_010efc(hb + blk, cx, ..., 0xc9, 2)` ✓ `f.c:24951-24954` ✓ kind 2,
   type_idx 0xc9 = 201 ✓). docs/241 §1 called it "the hit victim" ✓ corrected ✓

## 2. Where damage actually lands (shipped `fn_01441e` 0x148e7-0x14917, disassembled)

```
0x148ea movsx eax, word [B + 0x23]        ; the pending term ("debt")
0x148f0 test eax, eax ; jne 0x1491c       ; debt != 0 -> the other branch
0x148f8 sub word [B + 0x29], ax           ; HP -= dmg
0x14903 movsx edx, word [B + 0x29] ; test ; jg 0x14917      ; still alive
0x1490a mov word [B + 0x29], 0
0x14913 mov byte [B + 0x30], 1            ; THE settle stamp -- only when HP reaches 0
0x1491c:  if (B != player && B[+0x56] != 0) -> 0x14932: sub dword [B + 0x56], dmg   ; the reserve pool
```

The port models all of it (`f.c:25254-25286`, including the `+0x56` pool and the debt-drain that ends in
`+0x30 = 2` ✓ then `+0x30 = 1` if HP goes <= 0 ✓), so this is not a missing branch ✓

New `DMG` witness (`NIGHT_HITTRACE`, `f.c:25248`), 150 s, `/tmp/rig/h3`:

```
32 DMG events:  24 on the player (kind 0)          e.g. debt=100 hp=1000 dmg=1 / debt=99 dmg=3 / debt=96 dmg=6
                5 on enemies type 31               e.g. kind=1 ty=31 debt=35 hp=0 dmg=2 flag=0
                3 on enemies type 36
```

So an enemy's HP is `+0x23 = 35` with `+0x29 = 0` ✓ (the shape the shipped uses ✓), and with the contacts the
port actually lands (8 enemy hits in 150 s, dmg 1-6 ✓) the debt never drains to <= 0 ✓ -> the debt-drain block
that would write `+0x30 = 2` and then `+0x30 = 1` never runs for an enemy ✓ -> no settle flag on a victim ->
neither award producer runs (docs/240 §2) ✓

## 3. Why so few contacts land: the player is not where the shipped's player is

`fn_013c4d` funnel, 150 s, `/tmp/rig/h2` (444 calls, kinds attributed to the gate that killed each call):

```
A=player B=enemy (404):  GATE4 (the dl Y window, dl=7) 270  |  GATE5 (X box) 114  |  GATE6 (Y box) 9  |  GATEOK 11
A=enemy  B=player ( 40):  GATEOK 20  |  GATE5 12  |  GATE2 (intangible) 3
PASS3 (fn_013c4d did not return 1): 270 + 114 + 9 = 393 -- the 31 GATEOK calls DO return 1
```

Player Y time series, whole dumps, `scripts/rig/player_y.py` (order kept, run-length encoded):

```
shipped (974 states, 972 with a player record):
   [172 x46] [166] [162 x3] [167 x18] [157 x12] [154] [147 x12] [144] [139 x878]     <- monotone descent, then parked
   value counts: 139 x878, 172 x46, 167 x18, 157 x12, 147 x12, 162 x3, 166 x1, 154 x1

port (1500 states, 813 with a player record):
   [172 x365] [164 x17] [158 x15] [163] [164 x7] [161] [152] [150 x7] [153] [158 x14] [156] ...
   value counts: 172 x365, 139 x253, 158 x75, 143 x33, 164 x32, 151 x16, 159 x16, 150 x8
```

The shipped's player descends 172 -> 139 and then **does not move for the remaining 878 samples** ✓ 139 is the
room's lower bound `mn = DAT_00063354[+0x12] + camY` that `fn_01586b` computes (`NIGHT_YCLAMP` witness:
`mn=139 mx=195 room[+0x10]=0 camY=0 rows=9` ✓ `/tmp/rig/y1` ✓) ✓ and the spawn table
`DAT_00052c3c[room*4 + (stage-1)*0x28]` (the walk line ✓ transcribed at `f.c:5363`, used at `f.c:5519` ✓) is the
value the shipped writes at 0x376a4 ✓ The port's player instead **oscillates**: the `NIGHT_YCLAMP` witness shows
its delta alternating `d=-1 / d=+1` frame by frame around 171-177 ✓ `/tmp/rig/y1` ✓ so it never commits to the
walk line ✓

With the player at 139 and enemies between 141 and 186 (§1 caveat about how that range was measured ✓),
`ay + dl` = 139 + 7 = 146 overlaps the near enemies ✓; with the player at 172 the same gate needs an enemy at
165-179 ✓, which the census says is rare ✓ -> 270 of 404 player attacks rejected ✓

## 4. Open question, narrowed to one line

The defect is no longer "the hit gates" (they are byte-faithful ✓ docs/238 §1) nor "the award table" (it loads
and matches row for row ✓ docs/237 §2) nor "the settle chain" (it is modelled ✓ §2). It is **the sign/constancy
of the player's vertical delta** in the per-frame mover: the shipped produces a monotone descent to the room's
lower bound and holds it; the port produces `+1 / -1` alternation and holds ~172.

Next measurement (specified, not yet run): for the player record only, log every `fn_01586b` call with its
`mv_b` **and the state handler that produced it** (`fn_01776f` / `fn_017927` / `fn_017ca6` ✓ `f.c:16499-16519`)
plus the shipped's own dy source at the corresponding call site (0x12e01-0x12e17: `edx = [e+0x11] << 2;
edx += [room + 0x75]; d = [edx]` ✓ i.e. the shipped indexes a per-frame delta table hanging off the room
record's `+0x75` pointer ✓), and check whether the port's `mv_b` comes from that same table or from the
state-handler models.

# 238 — Item 2b, third look: the Y gate is byte-faithful, the player's Y matches, and the shipped's enemies stand on two ground lines while the port's float across 32 of them

Continues `fix-exp-values` after docs/237.  Everything here is one route, both sides, no pacing claim (docs/234 §3).

## 1. The gate itself is faithful — re-read from the shipped bytes

`fn_013c4d`'s vertical test, shipped 0x00013d4a-0x00013daf:

```
0x13d4a  edx = [ebp-8] + [ebp-4] ; 0x13d52 al = byte [edx + 0x5d] ; -> [ebp-0x2c]        (spawned state)
0x13d58  [ebp-0x28] = [b + 0x0d]                                   (B's type index)
0x13d61  [ebp-0x24] = [b + 1]                                      (B's world X)
0x13d6a  eax = [b + 9] ; 0x13d73 eax += [b + 5] ; -> [ebp-0x20]    (B's Y + the +0x05 term)
0x13d79  edx = byte[[ebp-8]+[ebp-4] + 0x56]                        (dl, the allowance)
0x13d84  edx += [a + 9] ; 0x13d8d cmp edx, [b + 9] ; jl 0x13daf    ; reject if a[9]+dl <  b[9]
0x13d92  edx = dl ; 0x13da0 eax = [a + 9] ; 0x13da3 eax -= edx
0x13daa  cmp edx, [b + 9] ; jle 0x13db4                            ; reject if a[9]-dl >  b[9]
0x13daf  jmp 0x1434f                                               ; the "no hit" exit
```

The port (`port/src/f/f.c:24861-24876`) computes `dl = *(uint8_t *)(rec + rec_off + 0x56)`, `ay = a[+9]`,
`by9 = b[+9]` and rejects on `ay + dl < by9` or `ay - dl > by9` ✓✓ identical, including the fact that the summed
`b[9]+b[5]` value is kept for *later* use (`[ebp-0x20]`) and is **not** the gate operand ✓ So docs/195 §1a's
"the defect is placement, not threshold" now has the mechanism behind it: the comparison is right, the numbers it
compares are wrong ✓

## 2. What the two sides actually carry in those fields (same route, both dumps)

New tool `scripts/rig/ent_geom.py` (walks the 0x160-byte record blocks in both dump formats, entity gate from
docs/186 §1 plus a new "unused slot" filter: `x == y == 0 && state == 0` is not an entity ✓ 2 741 of 3 022 port
kind-0 samples were such slots ✓).  Basis: the canonical 150 s route `ep1_kill3.txt` (`/tmp/rig/g33`, shipped
413 states, port samples at 10 Hz), window 60-150 s:

```
shipped  kind 0 (player):   samples=136  y: 172 .. 172   (1 distinct value)
shipped  kind 1 (enemies):  samples=71   y: 193 x18, 150 x15   (+ 38 junk slots at 50331648)
port     kind 0 (player):   samples=281  y: 172 .. 172   (1 distinct value)      <- matches
port     kind 1 (enemies):  samples=336  y: 139 .. 193, 32 distinct values
                                     most frequent: 193 x107, 171 x39, 190 x35, 147 x33, 162 x27, 150 x25
port     kind 2 (reaction): samples=14   y: 179 .. 181
```

* the player's world Y is **172 on both sides** ✓ so the attacker's side of the gate is not the diverging half;
* the shipped's enemies occupy **two discrete Y values** (193 and 150 ✓ the two ground lines of the room) ✓;
* the port's enemies occupy **32 distinct Y values spread over 139-193** ✓✓ — that is exactly the 15-24 px
  separation docs/195 §1a counted on the 352 `GATE4` rejections (`172 - 150 = 22`, `172 - 155 = 17` ✓), and it is a
  placement defect in the *enemy*, not in the attacker and not in the threshold ✓

## 3. Why that is the item-2b blocker, with the counters

Same route, `NIGHT_ACTTRACE` census on the current build (`/tmp/rig/x2`, docs/237 §3):

```
fn_01441e walk      : kind 0..7 = 20 6 0 0 0 0 0 0      (+0x30 seen: 26 samples, all 0)
settle-flag writers : fn_035c5c calls=0   fn_01441e sets(+0x30=2)=0
exp award gates     : +0x30!=1 1224 ... reached 0
exp award path      : adds=0 committed=0
```

and docs/195's hit census on the same drive: `GATEOK 33`, `GATE4 352`, `GATE5 178` ✓ So the chain is:

enemy Y wrong (this document) -> `fn_013c4d` rejects the contact (352 at the Y gate) -> `fn_01441e`'s damage /
settle stage rarely runs (26 walk visits, every one with `+0x30 == 0`) -> no `fn_035c5c`, no award branch, so
neither the flat 5 (0x15054) nor the per-type table value (0x15067/0x15070, whose table *is* loaded correctly with
row 31 = (35, 2, 0, 10) ✓ docs/237 §1) is ever written -> EXP stays 0 and EXP-per-type is untestable ✓

That also keeps docs/161/162 (population -> damage), the kill-count regression (docs/203) and item 2b on one cause
rather than three ✓

## 4. Open sub-question the next measurement has to answer

Why do the shipped's enemies sit on 193 / 150 while the port's drift?

* `fn_010efc` (the constructor) receives the spawn Y as an argument (`f.c:1312` area reads the `.chr` row for the
  type ✓ and `fn_03e20f`'s spawn tails pass the Y) ✓ the shipped's `.chr` row 31 carries `+0x14 = 300`,
  `+0x18 = 150` (docs/237 §1 witness) ✓ — 150 is one of the two shipped ground lines ✓ candidate: the shipped
  *snaps* the spawn Y to a floor value taken from the row (or from the room record), the port passes the raw value;
* `fn_011827` / `fn_01194c` (the step dispatch and its reaction spawners) move entities by the pushed `dx/dy`
  (e.g. `0x119ad cmp [e+0x11], 0x3a` then `push 4; mov ebx,-4; mov edx,-10` at 0x119b6-0x119ca) ✓ — if the port's
  per-step gravity/ground clamp differs, the Y spread grows ✓

Next measurement: per-kind Y *time series* on both dumps for the 60-150 s window (already available from the two
dump sets) plus a `NIGHT_SPAWNTRACE` print of the Y each spawn passes to `fn_010efc` ✓ and the shipped-side
comparison via `wait ds` on the spawn-Y cell if a bare-displacement read exists (run
`imm_scan.py --imm <disp>` with the docs/237 §4 fix ✓).

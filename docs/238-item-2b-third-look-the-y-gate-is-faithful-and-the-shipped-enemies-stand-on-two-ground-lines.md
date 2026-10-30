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

**Sample-size caveat, stated before anyone quotes this table:** the shipped side contributes only **33 admissible
enemy samples** over the window (the shipped dump stores 12 record cells per state, 413 states, and 38 of the 71
kind-1 rows are junk slots), against **336** on the port side ✓ So "the port occupies 32 Y values" is solid, while
"the shipped occupies exactly two" is a *lower bound on the shipped's occupancy* -- it is what the shipped was
caught doing, not proof it never passes through 139-193 ✓ The comparison that is safe to quote is the one that does
not depend on sample size: **the shared ground value 193 is the port's single most frequent enemy Y (107 samples)
and the player's Y is 172 on both sides** ✓ so the excess Y values are the port's own ✓

Follow-up on the spawn side, since it is the obvious suspect and it is *not* the defect: the shipped's spawn call
(0x0003e444-0x0003e45e: `push 1` kind, `push [rec+1]` type, `ECX = [rec + 0x0d]` Y, `EBX = 0`,
`EDX = [ebp-0x10]` X, `call 0x10efc`) is what `e20f_spawn` does (`f.c:27001-27004`, same citations ✓✓) ✓ So the
Y handed to the constructor is read from the same field on both sides, and the excess spread must come from what
happens to the record **after** spawn (the step dispatch's movement / ground handling, `fn_01194c` 0x1194c and its
`fn_011827` reaction moves at 0x119b6-0x119ca), not from the spawn Y ✓

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

Where does the port's extra Y spread come from, if the spawn read is faithful (§3)?

* ruled out: the spawn Y argument -- `0x3e453 mov ecx, [rec + 0x0d]` vs `f.c:27002` ✓ identical;
* ruled out: the Y gate itself (§1, byte-faithful ✓) and the attacker's Y (§2, 172 on both sides ✓);
* open, in priority order:
  1. the per-step vertical movement / ground clamp: `fn_01194c` (the step dispatch, `f.c:26146`) and `fn_011827`
     (0x11827, whose caller pushes the dy -- e.g. `0x119b6 push 4; 0x119bd mov ebx,-4; 0x119c2 mov edx,-10` ✓).
     Compare the shipped's Y updates (`add [e + 9], ...` sites inside 0x1194c-0x1260d) with the port's model, and
     check for a ground clamp the shipped applies and the port does not;
  2. `fn_031f10(cur, 5)` / the state-queue paths that write Y when a state changes (0x149bd-c5 ✓ modelled at
     `f.c:25274+`);
  3. the room record's `+0x0d` content itself (would show up as a *constant* wrong Y, which §2 does not show ✓).
* measurement that decides it: a per-record Y trace on the port (`NIGHT_HITTRACE`'s `ay`/`by9` lines already give
  the pairs it tests ✓ 352 lines ✓) against a Y trace from the shipped dumps for the same room, and the shipped-side
  `add [e + 9]` writer census via `field_writers.py --disp 0x9` with the data filter docs/189 §3 requires ✓

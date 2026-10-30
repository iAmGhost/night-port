# 225 — Item 6: both anchor arms and both blit helpers are byte-faithful; what the port is missing is the *mirror flag* itself (8 shipped stores absent)

Gate: `gate3-side-glitch`, unfixed build, runtime witness `NIGHT_ENTGEOM` on `scripts/rig/drives/
ep1_leftedge.txt` (11976 composites in `/tmp/rig/le6.log`), static census
`scripts/audit/field_writers.py --disp 0x1b` (saved `/tmp/rig/fw1b.txt`).

## 1. The draw routine and its two arms, matched byte for byte

The entity sprite draw is `fn_012efa` (shipped 0x00012efa, port `f.c:14882-14950`), driven by the sweep
`fn_03203d` (`f.c:14965`).  Its arm selector is `cmp dword [ent+0x1b], 0` (guest 0x0001331b, port
`f.c:14922`: `flipw = dword[e+0x1b]`), and the same field's **byte** is the blit mode pushed to both blitters
(guest 0x00013331 / 0x000133dd: `mov al, byte [ent+0x1b]; push eax`; port `flip = byte[e+0x1b]`).

Per-sprite parallel arrays (read with the frame index `i = dword[e+0x17] & 0x7fffffff` scaled by 4):

| shipped field | meaning | port |
|---|---|---|
| `[ent+0x12b][i]` | W (sprite width = blit `col`, ECX) | `f.c:14915` |
| `[ent+0x12f][i]` | H (rows) | `f.c:14916` |
| `[ent+0x137][i]` | X (anchor column inside the sprite) | `f.c:14917` |
| `[ent+0x13b][i]` | Y (anchor row) | `f.c:14918` |
| `[ent+0x133][i] + [ent+0x127]` | RLE stream address | `f.c:14920` |

The two x formulas, verbatim:

```
forward (0x133a4-0x133bd):  EDX = [ent+1] - X[i] - DAT_000639dc              port: (ex - X) - camX      f.c:14921
mirror  (0x1344b-0x13479):  EDX = [ent+1] - (W[i] - X[i]) - DAT_000639dc     port: ex - (W - X) - camX  f.c:14923
```

and the row anchor, which docs/220 did not quote: `EBX = [ent+5] + [ent+9] - Y[i] - DAT_000639e0`
(guest 0x0001337d-0x0001339e; DS 0x639e0 is the scroll row) — port `((ey + ez) - Y) - camY` ✓ (`f.c:14921`,
`f.c:14925`).

Runtime census, `/tmp/rig/le6.log`, 11976 composites, one line per blit with the values the port actually used:

```
entsprgeom spr=7 W=27 H=79 X=10 Y=76 worldx=15 camx=0 face=0 dest=5 arm=forward tick=0 player=1
arm=forward 3712   (all face=0)   dest == worldx - camx - X  in 3712/3712
arm=mirror  8264   (all face=1)   dest == worldx - camx - (W - X) in 8264/8264
left-edge subset (player=1, worldx <= 20, camx=0): 10 composites, spr=7 (W=27, X=10) x6 face=0,
                                                   spr=9 (W=30, X=14) x4 face=1
```

So the port's geometry is not the defect (docs/220 §2 confirmed, and the row anchor matches too).

## 2. The clamp candidates are equivalent — closed with arithmetic

Three clamp candidates existed.  All three are behaviourally identical:

1. **bail threshold.**  Port bails at `dest_off > 0x13f` (`f.c:7584`, `f.c:7672`); the shipped bails at
   `dest_off > 0x140` (fn_03bebb's tests at 0x3bedc-0x3bf13).  The only value in between is `dest_off = 0x140`.
   There the shipped's forward setup gives `right = 0x140 - 0x140 = 0` (0x3bfc7-0x3bfdc), so every run hits the
   right-truncation arm with `ovf = col_acc + len`, `cnt = len - ovf = -col_acc <= 0`, and the draw is skipped --
   nothing is drawn, i.e. the shipped's own result at that x equals the port's early return ✓ no wrap.
2. **mutually exclusive vs independent clip setup.**  The shipped sets the left limit and *skips* the right
   clamp when `dest_off < 0` (`fn_03bb42` 0x3bc3d-0x3bc4c `jmp 0x3bc66`; fn_03bebb 0x3bfb6-0x3bfc5
   `jmp 0x3bfdf`), while the port computes both (`f.c:7611-7614`, `f.c:7700-7703`).  The port's right clamp
   only fires when `dest_off + col > 0x140`; with `dest_off < 0` that needs `col > 0x140 - dest_off > 0x140`,
   i.e. a sprite wider than the 320-column canvas, which no sprite record has (largest W in the census: 59).
   With `dest_off < 0` the port therefore also leaves `right = col` ✓ same value, same behaviour.
3. **run-consumption when `start_row < 0`.**  Shipped 0x3bf13-0x3bf82 consumes `-start_row` rows' run lists
   (`cursor += 2` per run-count byte, `cursor += col_byte + 2` per run, `dec cursor`) and then
   `rows += start_row; start_row = 0` ✓ port `f.c:7595-7604` reproduces it including the `col_byte+2` quirk ✓.

What is *not* a candidate any more: the column walk is forward in the shipped's forward arm
(`0x3bfee` row loop, `0x3c01d` run loop, `col_acc` accumulated at 0x3c036/0x3c215) and reversed in its reverse
arm (`db = base + row*0x140 + dest_off + col - 1` at 0x3c0e5-0x3c0f3, draws at `db - len - col_acc + 1` at
0x3c1a1-0x3c1aa, `db - right + 1` at 0x3c1d1-0x3c1d7) -- and the port's `mode == 1` branch (`f.c:7641-7663`)
matches all three arms and both setups (`left = dest_off + col - 0x140` at 0x3c103-0x3c10e, `col += dest_off`
when `dest_off < 0` at 0x3c113-0x3c11c) ✓.  The mirror is therefore *also* a blit-level reversal in the shipped,
and the port has it.

## 3. The divergence: nothing in the port sets the flag that picks the arm

`[ent+0x1b]` is the arm selector and the blit mode.  Census of every shipped instruction touching displacement
0x1b (`field_writers.py --disp 0x1b`, 28 functions) compared with each ported function's body:

| shipped writer | first site | stores | port body | port writes +0x1b? |
|---|---|---|---|---|
| fn_0113e7 | 0x11511 | 6 | `f.c:15303` | yes |
| fn_011827 | 0x118ac | 2 | `f.c:14388` | yes |
| fn_013908 | 0x13977 | 2 | `f.c:15241` | yes |
| fn_013c4d | 0x141fe | 10 | `f.c:24753` | yes |
| fn_01441e | 0x14f35 | 1 | `f.c:24991` | yes |
| fn_018e88 (behaviour handler tail) | 0x19e21 | 2 | `f.c:16479` | yes (its 0x018e61/0x018e78 facing pair is faithful) |
| fn_01aec8 ... fn_02fe1e (9 behaviour handlers) | 0x1be81..0x31a68 | 2 each | `f.c:17163..20880` | yes |
| fn_03272f | 0x328ff | 2 (`=1`, `=0`) | `f.c:27233` | **no** |
| fn_0376c8 (player facing) | 0x378ad | 2 | `f.c:5547` | yes |
| fn_0381d7 | 0x38232 | 1 | `f.c:15475` | yes |
| fn_03d244 (room init) | 0x3d38a | 1 | `f.c:26728` | yes |
| fn_03e20f (script-command pump) | 0x3e361 | 6 (`= edx`) | `f.c:26932` | **no** (only the spawn-path store) |

Cite-level confirmation (the port labels each modelled line with its guest address): none of
`0x3e361, 0x3e483, 0x3e58f, 0x3e6bc, 0x3e804, 0x3e932, 0x328ff, 0x32923` appears anywhere in `f.c`; the single
modelled store in that family is `f.c:26926` `ent+0x1b = rec+0x05` citing 0x0003e2f6 ✓ the spawn-path copy of
the record's sub-command.  Within fn_03e20f's own modelled span (cited 0x3e20f-0x3ed40) those six addresses fall
in uncited gaps (0x3e2f6..0x3e367, 0x3e453..0x3e486, 0x3e5b0..0x3e6c9, 0x3e769..0x3e825, 0x3e88f..0x3e94f).

Shipped semantics of one of the missing stores (0x3e35b-0x3e367):

```
0x3e35b  mov  edx, [eax + 5]        ; the command record's sub-command / value
0x3e35e  mov  eax, [ebp - 0x24]     ; the target entity
0x3e361  mov  [eax + 0x1b], edx     ; <-- sets the mirror flag AND the blit mode byte
0x3e367  mov  [eax], 0              ; clears the record opcode
```

`fn_03272f`'s pair (0x328ff `= 1`, 0x32923 `= 0`) is the same flag written by the entity behaviour pump --
and fn_03272f is already known as the function whose seven stores to DS 0x62d5c the port omits (docs/179).

**Consequence, and the one-blit localization the contract asks for.**  The arm selector at 0x0001331b /
`f.c:14922` reads that dword, and the mode byte passed to `fn_03bb42` (copy blit) / `fn_03bebb` (fill blit) is
the same field.  A limb whose flag the shipped would have set to 1 by a script command keeps 0 in the port, so
the port takes `fn_012efa`'s arm B (`f.c:14924`, `dest = ex - X - camX`, mode 0 = forward runs) where the
shipped takes arm C (`f.c:14926`, `dest = ex - (W - X) - camX`, mode 1 = reverse runs).  The resulting
displacement of the drawn limb is `W - 2X` pixels, measurable per sprite from the witness census:

```
spr=1..5 (enemy bodies)  W=42, X=14 -> 14 px      W=41, X=13 -> 15 px
spr=0                    W=59, X=31 -> -3 px      W=53, X=24 -> 5 px
spr=7 (player, left edge) W=27, X=10 -> 7 px      spr=9 (left edge) W=30, X=14 -> 2 px
```

plus the loss of the run reversal (`db - len - col_acc + 1` vs `db + col_acc`), which flips the limb's internal
shape as well as its position ✓ the observed "leg/arm drawn on the right side".

## 4. Verdict for item 6

* **DETECTED-AND-LOCALIZED** (port half, rig-only): the limb draw takes the wrong arm of `fn_012efa` because
  the flag it selects on is never written by the port's models of `fn_03e20f` (6 stores, guest 0x3e361/0x3e483/
  0x3e58f/0x3e6bc/0x3e804/0x3e932) and `fn_03272f` (2 stores, 0x328ff/0x32923).  The blit whose output differs is
  `fn_03bb42` at `f.c:14924` vs `f.c:14926` (and `fn_03bebb` for the 0x14-frame fill pulse), quantified as a
  `W - 2X` displacement with the run reversal dropped.
* The three clamp/wrap candidates -- bail threshold `0x13f` vs `0x140`, mutually-exclusive vs independent clip
  setup, `start_row < 0` run consumption -- are **audited and equivalent** (§2), so the contract's expected
  "clamp the shipped applies and the port does not" is disproved rather than filled: the shipped's clamp
  structure is named and shown to produce the same pixels.
* **RIG-GAP**: no shipped-side *pixel* pair for the left-edge limb.  The shipped dump and the port dump do not
  share a presentation basis (docs/202, docs/205), and the shipped side has no in-process witness, so the
  cross-side evidence here is the code census plus the port's own `entsprgeom` witness (11976 lines, 3712
  forward / 8264 mirror).  A shipped-side pixel basis is the new oracle the fix phase should build.

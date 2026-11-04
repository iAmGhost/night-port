# 212 — Item 6 probes: a left-edge attack drive, the mirror-selector census, and the two arms' arithmetic verified byte-for-byte

## 1. A drive that puts the player at the left edge and attacks there

`scripts/rig/drives/ep1_kill3.txt` truncated at 120 000 ms, then `ep1_leftedge.txt` appends eight long left holds
(`key left down` 1200 ms / `up` 300 ms) and fourteen left-tap + attack pairs (`key left` 250/150 ms then `key lctrl`
120/1300 ms), last event 156 680 ms.  Measured on the port (`/tmp/rig/le3`, `NIGHT_ENTDBG=1` ent_dump, 200 player
samples): `cameraX = 0` for the whole run, minimum player world x = **15**, and every sample taken at world x = 15
has `face = 1` (the mirror selector).  So the rig *can* now reach item 6's position.

## 2. The shipped facing bit, measured out of a shipped heap block

`/tmp/rig/ents-orig/orig` dumps 63 pointer-cell blocks of 352 B each; the block for `DS 0x53c7c` (the player
record) gives `+0x1b` directly across all 974 states:

```
overall:            +0x1b = 1 in 648 states, 0 in 326
while world x decreasing (walking left):   1 x57 / 0 x33
while world x increasing (walking right):  0 x72 / 1 x26
```

i.e. `+0x1b` is the facing/mirror selector and it tracks movement direction.  The port's own census
(`/tmp/rig/le3`: 200 player samples, 0 x165 / 1 x35, `face=1` at the left edge) agrees in *meaning*; the ratio
difference is a route/duration difference, not yet a defect, so it is recorded as unattributed.

## 3. Both mirror arms and the selector are faithful (this is what item 6 is NOT)

`fn_012efa`'s selector and arms, shipped bytes against the port model (`src/f/f.c:14852-14872`):

```
0x12fca  cmp dword ptr [eax + 0x1b], 0 ; jne 0x13163      ; selector is a DWORD compare, not a byte
0x133ad  mov  edx, [edx + 0x137] ; add edx, eax           ; X[spr] from the +0x137 parallel array
0x133b8  mov  eax, [eax + 1]                              ; ent world x
0x133bb  sub  eax, [edx]                                  ; - X[spr]
0x133bd  sub  eax, [0x139dc]                              ; - cameraX
0x133c8  call 0x3bebb                                     ; forward arm:  x = ex - X - camX

0x1344b  mov  eax, [ebp-0x10] ; shl eax,2 ; edx = [e+0x12b] ; eax = W[spr]
0x13462  esi = [e+0x137] ; edx = X[spr]
0x1346d  eax = [eax] ; sub eax, [edx]                     ; W[spr] - X[spr]
0x13474  edx = [e+1] ; sub edx, eax                       ; ex - (W - X)
0x13479  sub edx, [0x139dc]                               ; - cameraX
0x13482  call 0x3bebb                                     ; mirror arm:   x = ex - (W - X) - camX
```

The port's `flipw = *(int32_t *)(e + 0x1b)`, `(ex - X) - camX` and `ex - (W - X) - camX` reproduce all three.
`field_writers.py --disp 0x1b` names the writers on both sides' paths: `fn_02fe1e` (0), `fn_03272f` (1 at 0x000328ff,
0 at 0x00032923), `fn_0376c8` (0 at 0x000378ad, 1 at 0x000378cf), `fn_034635`, `fn_0381d7`, `fn_03d244`, and
`fn_03e20f` (six `= edx` stores).

Composite census (`/tmp/rig/le2`, `NIGHT_ENTSPR=1 NIGHT_ENTBUDGET=4000 NIGHT_ENTQUOTA=1500`, 3 190 composites):

```
mode overall: 0 x1862   1 x1328
gun.spr   mode=0 1401   mode=1   99
ena-1.spr mode=1 1086   mode=0  414
mark.spr  mode=1  143   mode=0   47
```

The player's own weapon composite is overwhelmingly the *forward* arm even though the drive spends most of its time
walking left -- consistent with "the limb is drawn on the wrong side at the left edge", but with the arms and
selector verified faithful the mismatch has to come from the data the arms consume.

## 4. One real (narrow) divergence found in the blitter while checking this

`fn_03bb42`'s clipping is **mutually exclusive** in the shipped code (`src/f/f.c:7665-7673` vs guest
0x0003bc43-0x0003bc63):

```
0x3bc43  mov eax,[ebp-0xc] ; mov [ebp-0x14],eax ; neg [ebp-0x14]     ; dest_off < 0 -> left = -dest_off, no right clip
0x3bc4e  (dest_off >= 0 branch)  if (dest_off + col > 0x140) col = 0x140 - dest_off   ; right clip, left stays 0
```

while the port computes `left` and `right` independently and also bails at `dest_off > 0x13f` where the shipped
bails only at `dest_off > 0x140` (0x0003bb6a `cmp [ebp-0xc], 0x140 / jg`).  The reachable differences are a sprite
wider than the 320-px canvas with `dest_off < 0` (shipped never right-clips it; the port does) and `dest_off == 0x140`
(shipped proceeds and draws zero columns; the port returns).  Recorded as a divergence, not yet connected to item 6.

## 5. Status and the next measurement

`gate3-side-glitch` is not detected yet.  Excluded by measurement: the facing bit at the left edge, the
forward/mirror arm choice, and both x formulas.  The next measurement is the data the arms consume: dump the player's
`+0x12b` (W), `+0x12f` (H) and `+0x137` (X) parallel arrays for the attack-action sprite indices on the port, and
compare them against the same arrays decoded from the shipped `.act`/`.spr` resources (stage-independent, the
`scripts/playtest_verify_spr.py` path), because a sprite whose `X` sits on the far side of its `W` flips the visual
side of the limb without any code divergence at all.

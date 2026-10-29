# docs/149 — the walk quantum: shipped ±60 px/s, port ±70 px/s with a port-only ±80 band

docs/144 reported velocity bands on a build whose action trip count was still wrong; docs/147 fixed the trip
count and docs/148 narrowed the camera question to "why does the port's player reach the camera-follow
threshold". `scripts/rig/walk_rate.py` measures the thing directly: displacement per second of the player's
`+0x01` field, attributed only to sample pairs where the player's state is the same at both samples (so a rate
is attributed to a state, not to a transition), with `DAT_00053c68 == 0` pairs and >1.5 s gaps dropped.

## 1. Measured on the anchored route `/tmp/rig/g12` (`ep1_kill3`, 150 s)

| side | cadence | state 0x02 (walk) pairs | median | 25-75 pct | 10 px/s bands (count) |
|---|---|---|---|---|---|
| shipped | 305 ms | 75 | **60.7 px/s** | -63.5 .. +64.7 | -60 ×25, +60 ×22, **+70 ×17**, -70 ×11 |
| port | 100 ms | 127 | **40.3 px/s** | -70.0 .. +70.0 | **+70 ×48**, -70 ×27, **+80 ×7**, -80 ×6, +30 ×5, +50 ×5, +40 ×5 |

Other states are stationary on both sides (median 0.0 px/s for `0x00`, `0x01`, `0x33`, `0x34`, `0x35`), so the
whole motion budget of this drive is the `0x02` walking state, and the port's quantum there sits one step
higher than the shipped one. docs/144's "60-69 shipped vs 70-79 port, plus a port-only 80-89 band" is
**reproduced after docs/147**, so the trip-count fix tightened the *duration* of actions but not their
*speed*.

Sanity anchors that the measurement is reading real shipped constants:

* the port's camera-relative extremes land exactly on shipped-derived numbers: `190 = 0xbe` (the right-arm
  threshold in `fn_03d4e4`, 270 samples sit exactly there) and `305 = cam + 0xf + 0x122` (the clamp window of
  the first arm of the mover, 50 samples);
* shipped's camera-relative census peaks at 155/156 — below every clamp bound and below the 190 follow
  threshold, i.e. the shipped player is stopped by something other than a clamp or the camera policy.

## 2. The movement primitive is faithful; the step size is the open variable

Everything that changes an entity's x goes through one mover (guest `0x1547b-0x15860`, port `f.c:8938-9000`):

```c
if (e[+0x0d] == 0)       lo = cam + 0xf,  hi = lo + 0x122;   /* 0x15485-0x154da: [cam+15, cam+305]  */
else if (e[+0x17] == 2)  lo = cam - 0x64, hi = lo + 0x208;   /*                [cam-100, cam+400] */
else                     lo = cam + 0xa,  hi = lo + 0x12c;   /*                [cam+10,  cam+302] */
...
((struct sa32 *)((e + 1)))->v += d;                          /* 0x156ed-0x156f3 */
if (x < lo) { x = lo; ... } else if (x > hi) { x = hi; ... } /* 0x156f6-0x157c4 */
```

The clamps are camera-relative and transcribed exactly; **`d` is an argument** — the step is chosen by the
caller, and the caller's choice is what differs. Only three sites in the whole port store to `+0x01` outright:
the constructor (`0x10f38-3e`), the scripted mover `fn_0381d7` (`0x38255-72`), and the camera-anchor
placements (`0x3765c-6d`, `player[+0x01] = cam - 0x1e` or `cam + 0x15e`); everything else is `+= d`.

While auditing, `fn_0381d7`'s frame-count source was checked against the shipped body and is faithful, and it
clarifies a field that docs/147 made ambiguous:

```
0x38291: mov     eax, dword ptr [ebp - 0x20]   ; record, via [player+0x123] -> +4 -> +8
0x38294: cmp     edx, dword ptr [eax]          ; counter vs dword at record + 0x00
0x38298: mov     dword ptr [eax + 0x11], 0     ; reset when counter >= it
```

So there are **two** length fields in play and they are not interchangeable: `fn_012616` advances a chained
action using `byte[record + 0x5c]` (docs/147), while `fn_0381d7` loops `steps` times resetting on
`dword[record + 0x00]`. The port has both right.

## 3. Next measurement (the one that names the defect)

1. `scripts/audit/call_sites.py` for the mover (`fn_0154xx`): list every caller and the instruction sequence
   feeding its step argument, and see whether `d` comes from a record field, a table, or a computed value
   (the mover itself contains a `+ (prng_next() % 0x29) - 0x14` jitter at 0x15774 / 0x1582d that is applied
   only to non-player entities in state 0x16).
2. A port-side witness counting `d` per call, bucketed by caller, compared against the shipped velocity
   census above: the shipped quantum is ~±60 px/s and the port's is ~±70, so a witness that shows `d` stepping
   by 1 at ~60 Hz in the port and ~43 Hz in the shipped run (or `d = 1` vs `d = 1.5`) localises it in one number.
3. What stops the shipped player at camera-relative 156: candidates are the `clamp_tab_5c8bc` blocking loop at
   0x154f0-0x1562d (which rewrites `d` to a wall offset and can set state `0x20`), the entity's own
   `+0x0d` field selecting the clamp arm, or the action record's movement bytes.

## 4. Reproduce

```sh
OUT=/tmp/rig/g12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 150
python3 scripts/rig/walk_rate.py --orig /tmp/rig/g12/orig --port /tmp/rig/g12/port
python3 scripts/rig/cam_policy.py --orig /tmp/rig/g12/orig --port /tmp/rig/g12/port   # docs/148
uv run --with capstone scripts/audit/disasm.py --entry 0x15470 --end 0x15870          # the mover
uv run --with capstone scripts/audit/disasm.py --entry 0x38235 --end 0x382f0          # fn_0381d7's count
```

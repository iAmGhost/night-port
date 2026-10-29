# docs/148 — on this drive the shipped camera never scrolls at all; the port's player crosses the follow threshold

docs/142 measured the camera symptom, docs/143 identified the block behind `DAT_00063354` as a clamp-bounds
struct and queued "read the guard's fields from the right object". `scripts/rig/cam_policy.py` now reads every
input of the shipped camera policy step (`fn_03d4e4`, guest 0x0003d4e4) out of both dumps and evaluates the
gate per sample. On the anchored route `/tmp/rig/g12` (`scripts/rig/drives/ep1_kill3.txt`, 150 s) the answer is
sharper than expected.

## 1. Measured, signed, on one drive

| input of `fn_03d4e4` | shipped (424 samples, 4 Hz) | port (1709 samples, 10 Hz) |
|---|---|---|
| `DAT_000639dc` camera x | **0 in 424/424**, sample-to-sample deltas: none | 0 in 822, then 231 / 298 / 73 / 186 / 191; **max 298**, deltas dominated by +7 (×37) and -7 (×16) |
| `DAT_000639e0` camera y | 0 everywhere | 0 everywhere |
| `DAT_000639e4` (byte gate inside both scroll helpers, 0x3cb5d / 0x3cbf6) | 0 everywhere | 0 everywhere |
| right arm `room[+0x3b]==0 && player[+0x01] - cam > 0xbe` | **open 0 times** | **open 175 times** |
| left arm `room[+0x3a]==0 && player[+0x01] - cam < 0x82` | open 245 times | open 1029 times |
| player `+0x01` (x) | 0 .. **156**, clusters at 92/93 and 155/156 | 0, 90, 263, **-30**, min 0 max >190 values |
| camera-relative `player[+0x01] - cam` | 155, 92, 156, 93, 90 | 0, **190** (×270), 90, -30, **305** (×50) |
| clamp `+0x00` / `+0x08` (room span) | 0 / 1600 | 0 / 1600 |
| clamp `+0x2a` / `+0x2e` / `+0x32` / `+0x36` | **0 in 424/424** | 232 (×567), 0, 0, **617** (×531) |
| clamp bytes `+0x3a` / `+0x3b` | 0 / 0 | 0 / 0 |
| `DAT_00053c68` entity count | 1..5 | 0 (×544, intro/slide-show), 1..7 |

## 2. What that says

The shipped side's camera **does not move at all on this drive**: its right arm never opens, because the
shipped player's camera-relative x peaks at 156 and the arm needs `> 190` (`0xbe`). The port's player reaches
190 (and 305), the arm opens 175 times, `fn_03cb38` adds 3 or 1 per frame, the camera reaches 298, and every
camera move calls `fn_03d75c(cam - prev)` — the stage-record decay loop — which therefore runs **hundreds of
times in the port and zero times in the shipped run** of the same drive.

The question is no longer "which clamp bound is wrong". It is: **why does the port's player get pushed to the
camera-follow threshold when the shipped player stops 35 px short of it?** That is a locomotion question, and
docs/147's trip-count fix (counter max 88 → 69 against shipped 63) tightened but did not close it — consistent
with docs/144's remaining velocity bands (60-69 px/s shipped, 70-79 px/s port, plus a port-only 80-89 band).

## 3. Retracting the "617 bound" reading

docs/142 §4 and docs/143 left open whether the port's `617` at clamp `+0x36` (where shipped holds 0) was a
plausible direct cause of a camera that scrolls to 298 instead of parking. The shipped write site settles it:

```
0x3e98e: cmp     eax, dword ptr [0x139dc]   ; subj[+0x09] vs cam
0x3e994: jge     0x3e9ab                    ; subj[+9] >= cam  -> do not write the bound
0x3e996: mov     edx, dword ptr [0x13354]   ; DAT_00063354  (the clamp struct)
0x3e99f: mov     eax, dword ptr [eax + 9]
0x3e9a2: mov     dword ptr [edx + 0x2a], eax
```

The bound is only written when the record's position is *left of the camera*. While the shipped camera is 0 and
the player's x is 92..156, that condition never holds — so the shipped side's zeros are **not a missing write,
they are the absence of the preconditions for the write**. The port's 232 / 617 are a *consequence* of its
camera having already drifted, and `left_veto_by_2a` firing 230 times in the port (0 in shipped) is feedback
inside the same loop. Treating 617 as the cause would have been reading an effect as a cause.

## 4. A rig defect found while measuring, and the noise that is not evidence

* **Signed vs unsigned.** The first pass compared the raw unsigned dword reads, which made the shipped player's
  `-30` look like `4294967266` and reported "shipped right arm open 27 times". The guest compares signed dwords
  (`0x3d512-25`), so the script now sign-extends (`s32`) before every comparison: shipped right arm open
  **0** times. A gate census is only as good as the width and signedness of the comparison it transcribes.
* **15 samples of garbage in the port** (`clamp+0x2a = 0x7EF7EF03`, `+0x2e = 0x3000000`, and 79 samples where
  the player record's state field reads `0x2D2D2D25`, i.e. ASCII) and 544/1709 samples with `DAT_00053c68 == 0`
  (the port sitting in the slide-show/intro, which the shipped run of this route does not). Per docs/143's rule
  these are **not** evidence of anything until a live witness in the port says the same thing as the dump.

## 5. Next measurement

1. Walk-displacement differential: the per-frame delta of `player[+0x01]` on both sides during the same
   action state (docs/144's velocity bands, now with the corrected trip count), to see whether the port moves
   the player further per action or runs the action more times.
2. Census of `fn_03d75c` calls on the port side against the shipped zero, and whether the shipped stage-record
   decay is driven by some other caller.
3. A live witness for the 15 garbage samples before they are read as a defect.

## 6. Reproduce

```sh
OUT=/tmp/rig/g12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 150
python3 scripts/rig/cam_policy.py --orig /tmp/rig/g12/orig --port /tmp/rig/g12/port
uv run --with capstone scripts/audit/disasm.py --entry 0x3d4e4 --end 0x3d600   # the policy step
uv run --with capstone scripts/audit/disasm.py --entry 0x3cb38 --end 0x3cc60   # the two scroll steps
uv run --with capstone scripts/audit/disasm.py --entry 0x3e960 --end 0x3ea10   # the bound write
```

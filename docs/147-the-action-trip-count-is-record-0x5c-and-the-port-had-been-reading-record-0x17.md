# docs/147 — the action trip count is `record[+0x5c]`, and the port had been reading `record[+0x17]`

docs/144 measured the divergence head: the port's player travels 603 px where the shipped player never passes
217, and the camera scrolls to 298 where the shipped camera never leaves 27 (docs/142). docs/145 removed two
candidate explanations. docs/146 §12 measured a third symptom: the port's player frame counter `+0x11` reached
88 with median 42, against shipped max 63 median 30. Reading the shipped instruction stream for the action
advance closes all three at once.

## 1. The shipped instruction stream

`scripts/audit/disasm.py --entry 0x1271e --end 0x12810` (run it as
`uv run --with capstone scripts/audit/disasm.py ...`):

```
0x1271e: mov     ax, word ptr [eax + 0x50]      ; hold countdown
0x12729: jne     0x12a80                        ; countdown running -> other branch
0x12732: inc     dword ptr [eax + 0x11]         ; frame counter += 1
0x1273a: mov     dl, byte ptr [eax + 0x5c]      ; THE RECORD'S frame-count byte   <-- [ebp-0xc] = record
0x12740: cmp     edx, dword ptr [eax + 0x11]
0x12743: jg      0x12751                        ; record length > counter -> no state bump
0x12748: cmp     dword ptr [eax + 0x17], 0xab   ; state window 0xab..0xad
0x12764: inc     dword ptr [eax + 0x17]         ; advance the chained state
0x1276a: mov     dword ptr [eax + 0x11], 0      ; and RESET the counter
```

The same `byte ptr [record + 0x5c]` load appears for all three chained windows (0x1273a, 0x12776, 0x127b2) and
again for the `e[+0x31]` test at 0x127fd. `[ebp-0xc]` is the action record (`rec = RECST(state)`), so the
shipped trip count field is **`record[+0x5c]`**.

## 2. What the port had

Four sites in `fn_012616` read the trip count out of the record at **`+0x17`** instead of `+0x5c` — the same
number that is the *entity's* state field, applied to the record. Fixed to `record[+0x5c]`:

```c
if (((int32_t)*(const uint8_t *)(rec + 0x5c) <= ((struct sa32 *)((e + 0x11)))->v)
    && (0xaa < ((struct ua32 *)((e + 0x17)))->v) && (((struct ua32 *)((e + 0x17)))->v < 0xae)) {
    ((struct sa32 *)((e + 0x17)))->v = ((struct sa32 *)((e + 0x17)))->v + 1;
    ((struct ua32 *)((e + 0x11)))->v = 0;
}
```

The three windows' state bounds were already faithful (`0xab..0xad`, `0xb5..0xb8`, `0xbf..0xc7` against the
shipped `jae`/`jb` pairs at 0x12748/0x12756, 0x12784/0x12792, 0x127c0/0x127ce) — the wrong byte was the only
defect in the block.

## 3. Red -> green, same drive (`scripts/rig/drives/ep1_kill3.txt`, episode 1)

Player record census (`scripts/rig/frame_counter.py`, offsets from the constructor's store set, docs/125):

| measure | port before | port after | shipped |
|---|---|---|---|
| `+0x11` frame counter max | 88 | **69** | 63 |
| `+0x11` median | 42 | **31** | 30 |
| counter > 3 share | 1198/1544 | 185/263 | 366/488 |
| player state census top | `0x1`, `0x2`, **`0x15` ×168** | `0x1`, `0x0`, `0x2`, `0x33`, `0xc`, `0xb` | `0x1`, `0x2`, `0x33`, `0x35`, `0x0`, `0x34`, `0xc`, `0xb` |
| camera `DAT_000639dc` max | 298 (74 distinct) | **158** (20 distinct, mode 0 in 328 samples) | 27 |
| `NIGHT_DEATHTRACE` player death | 1 (tick 7066, hp 998) | **none** | none |

The state `0x15` census — the reaction state that the reaction table maps from hit code 11, which docs/137
showed is the only way the player ever gets marked dying — is **gone from the player's census**, and with it
the death that docs/135 identified as the trigger for the CONTINUE countdown (`g_dat_53100` 3 → 2, docs/134 §6)
and the extra CD selection (track index 12 at 236.5 s, docs/135 §3).

## 4. What this closes, and what it does not

Closed by this one byte, measured on the rig:

* the player's action running ~40% too long (docs/146 §12), which was the magnitude behind docs/144's
  per-frame displacement bands (60-69 px/s shipped vs 70-79 px/s port, plus the shipped-absent 80-89 band);
* the spurious player death and its whole downstream chain (CONTINUE countdown, extra CD track selection) —
  `NIGHT_DEATHTRACE` now reports deaths but no player death, matching the shipped side;
* the player being hit with reaction code 11, i.e. the "stuck when hit mid-kick" symptom family (docs/111's
  reaction table is unconditional and now selects a much narrower state set on this drive).

Still open, honestly measured:

* **camera 158 vs 27.** Better by roughly 2×, still an order of magnitude off. The camera scroll driver
  (`fn_03cb38` / `fn_03cbd9`, clamped by the `g_dat_63354` clamp-bounds struct, docs/143) has not been
  re-measured against the shipped side since this fix, and the scroll guard reads `room[+0x32]`/`room[+0x36]`
  from `fn_03ed6f`'s room parameter, which is still not instrumented.
* the shipped `DAT_00063e14` command counter holding 67 while the port wanders (docs/139: the port has 14
  `rec[0]=0` stores where the shipped function has 12) — an independent defect in `fn_03e20f`'s arms.
* fallen-state coverage (`0x5b..0x66`): zero samples on both sides for the drives in hand (docs/118), so the
  mechanism is decoded but not yet exercised.

## 5. Reproduce

```sh
make -C port app
./port/test/night --res-dir ./NIGHT --script scripts/rig/drives/ep1_kill3.txt --warp-stage 1 \
    --rig-dump /tmp/rig/c1 --rig-hz 4 --frames 6000
python3 scripts/rig/frame_counter.py --orig /tmp/rig/g10/orig --port /tmp/rig/c1
NIGHT_DEATHTRACE=1 ./port/test/night --res-dir ./NIGHT --script scripts/rig/drives/ep1_kill3.txt \
    --warp-stage 1 --frames 6000 2>&1 | grep -i "PLAYER"
```

and for the shipped instruction stream:

```sh
uv run --with capstone scripts/audit/disasm.py --entry 0x1271e --end 0x12810
```

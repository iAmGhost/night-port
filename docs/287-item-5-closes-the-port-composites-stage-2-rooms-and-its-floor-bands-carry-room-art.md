# 287 — Item 5 closes: on a drive whose keys satisfy the shipped's own gates the port composites stage-2 rooms, loads every stage-2 file, and its floor bands hold room art (0.00 % pure black); the fault half is clean under ASan on that same route

## 1. The gate that my earlier drives could not pass

Two key gates sit between "room clear" and "stage-2 room on screen":

* **leaving the level-up screen** — `fn_038f3d`'s main loop is entered only by the tail and exits only when
  `DAT_00063e89` (Esc) / `DAT_00063eb2` (LShift) / `DAT_00063ebe` (RShift) is non-zero: shipped `0x3920a`, `0x39213`,
  `0x3921c`, `jmp 0x39a56` ✓ == port `f.c:26028` ✓ (docs/286 ✓)
* **advancing each slide card** — `fn_00035dfc` (docs/97 ✓, shipped `0x35e14-0x35eb6`, call sites `0x3b0ec`,
  `0x3b272`, `0x3b4e1` ✓) accepts **only** the four game-key bytes `0x63ea4` Enter / `0x63ea5` CTRL / `0x63ec0` ALT /
  `0x63ec1` Space, returns 1 on ESC ✓ — **Shift cannot advance a card** ✓

`ep1_roomclear2.txt` pressed Enter only until t=215350 ms ✗; `ep1_roomclear3.txt` v1 pressed *Shift* from 216 s ✗ —
so the port (which reaches these gates at ~230 s because of the pacing residual, ratio 0.46-0.59, docs/269) stalled in
the stage-2 **start slide show**: `2s1.pcx` loaded, `2s2.pcx`/`stage2.pcx` never requested, mode stuck at 640x480 ✗
The shipped passed the same gates at ~87-95 s, while the drive was still pressing Enter ✓ — the same drive, a
different arrival time ✓ That is the whole content of docs/224 / docs/269 / docs/285's "no stage-2 assets" symptom ✓

`scripts/rig/drives/ep1_roomclear3.txt` (v2) keeps a game-key stream for the entire window: Enter every 2 s,
Space every 4 s, Shift every 8 s (menu exit), Right every 12 s, from 216 s to 291 s ✓

## 2. What the port then does (non-ASan `/tmp/rig/s2e`, route `/tmp/rig/rm4`)

```
roomstart: stage=2 vesa=1 slides=2      (room_start re-entered with the episode counter already 2 ✓)
slide: card 1/2 wait / slide: card 2/2 wait
resource lookups: 2s1.pcx ✓ 2s2.pcx ✓ stage2.pcx ✓ stage2.wal ✓ stage2.stg ✓ night2.chr ✓
mode segments (rm4): 640x480 0-13.9s | 320x200 13.9-43.8s | 640x480 43.8-90.3s | 320x200 90.3-100.5s
                     640x480 100.5-241.3s | 320x200 241.3-300.0s  frames=7091 changed=2283 ✓✓
```

`320x200` is the room composite view (`main.c:163-166 ✓`, `g_view_320x200 ✓` set by the room flip at `f.c:14659` /
the 0x3e80-dword copy at `f.c:15414` ✓): after the tail the port runs **58.7 s of 320x200 room compositing** ✓

Floor census on the port's stage-2 witnesses (`rm4/port`, 230 witnesses selected by the changed-frame share
(680+261)/3224 of the 320x200 changed frames, rows 100-199 = 7 360 000 sampled pixels):

```
rows 100-150 top colours (255,138,138)x285436, (255,44,56)x263432, (255,154,154)x258500
rows 150-200 top colours (255,195,195)x311464, (255,182,182)x296931, (255,162,166)x295555
pure-black 0.00 % of 7 360 000 px
```

So the "floor renders black" report does **not** reproduce on a basis where the port is actually in a stage-2 room:
the floor bands carry room art in every sampled pixel ✓

**Colour-basis caveat (standing rule reused).** Port witness bins and the shipped capture bins do not share a colour
basis: the shipped's stage-2 room bands are dominated by `(121,121,0)`, `(231,247,0)`, `(57,57,0)` (R==G, B=0 ✓) while
the port's are `(255,195,195)`, `(255,138,138)` (R=255, G==B ✓) — the same signature difference appears in each side's
*stage-1* rooms (`(61,45,0)` vs `(255,40,32)` ✓), so it is a dump-side channel/palette convention, not a stage-2
defect ✓ **Absolute cross-side colour comparisons stay inadmissible** (docs/202, docs/218); the admissible claim is
ink presence ✓

## 3. Fault half, re-confirmed on a route that really composites stage 2 (`/tmp/rig/s2f`)

`port/test/night-asan` (rebuilt with `make -C port asanapp` ✓) on `ep1_roomclear3.txt`, pinned clock, `--rig-hz 4`:

```
rc=0 ✓   grep -c "ERROR: AddressSanitizer|runtime error|Segmentation fault|SEGV" = 0 ✓
roomstart: stage=1 ... -> tail: stage=2 -> tail: levelup call done -> tail: stage reset done -> roomstart: stage=2 ✓
mode (320,200) t=234.9-333.3s frames=13249 changed=2540 ✓  (stage-2 room compositing under ASan ✓)
```

`"game main unwound (quit / fatal)"` at the end is the harness's frame-budget `longjmp` (docs/223 §2 ✓), not a fault ✓
Together with docs/223 / docs/268 (warp route: no fault, VESA shim returns 1, `DAT_00052f90` byte-identical ✓) the
contract's fault clause is satisfied on both route families ✓

## 4. Rig defects this step produced (gate3-regression items)

1. **`capture_original.py` died in the `rm4` screen-capture leg**:
   `ValueError: frame geometry is not self-consistent: 640x480 pitch=2560 format='BGRX32_ByteArray'
   paletted=False, 512000 payload bytes` ✗ — 512000 = **200 rows × 2560** ✓, i.e. the payload was a 320x200 buffer at
   8 bytes per reported column while the header claimed 640x480 ✓ (the docs/218 stride-2560 family ✓). Fix: derive
   rows/geometry from the payload size (or re-query `video/frame/info`) instead of trusting the reported `w,h` ✓
   Consequence: `rm4/orig_screen` stops at 63.4 s ✗ so rm4 supports DS/state claims only, not shipped screen claims ✓
2. **`report_audio.txt` is missing in route.sh runs** (`cat: /tmp/rig/rm4/report_audio.txt: No such file or
   directory` ✓) — the audio-step leg writes `report_audio_sfx.txt` / `report_audio_cd.txt` but the summary
   `cat` names `report_audio.txt` ✓
3. **Witness ordinal ≠ timestamp** ✓ `frames/NNNNNN_WxH.bin` names come from `rig_sink.c:446-448` (`g_witness` on
   signature change ✓, `witness_every` sampling ✓) and do not map 1:1 to `frames.bin` records ✓ Stage-window
   selection must use the changed-frame **share** of each mode segment, not a name arithmetic ✓

## 5. Item 5 verdict

* Fault: **not present** — `rc=0`, 0 ASan/UBSan/SEGV reports on the warp route (docs/223, docs/268) and on the
  natural room-clear route that composites 13249 stage-2 room frames under ASan (this run) ✓ The historical
  `bus error ./test/night` at stage-2 entry has not reproduced since docs/208 ✓
* Screen: **not present as reported** — the port loads `stage2.pcx/stage2.wal/stage2.stg/night2.chr` and composites
  stage-2 rooms whose floor bands contain art in 100 % of sampled pixels ✓
* Root cause of the *symptom the rig saw*: two key gates (`fn_038f3d`'s Shift/Esc exit ✓, `fn_00035dfc`'s four
  game-key card gate ✓) that a drive must satisfy, plus the pacing residual that makes the port arrive at them
  ~2.5× later in wall time than the shipped ✓

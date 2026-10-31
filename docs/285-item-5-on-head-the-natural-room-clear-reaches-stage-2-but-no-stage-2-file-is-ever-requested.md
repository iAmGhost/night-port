# 285 — Item 5: on HEAD the natural room clear reaches stage 2 but the port still never asks for a single stage-2 file; and my hand-rolled frames.bin parse was garbage

## 1. Reproduction on HEAD (no rig-warp, no stage poke), `/tmp/rig/s2a`

`./port/test/night --script scripts/rig/drives/ep1_roomclear2.txt --rig-dump /tmp/rig/s2a --rig-hz 10 --frames 18000`
with the pinned clock (`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`) ✓ `rc=0` ✓

```
memstat stage=2 room=1 frames=17766..17973 arena=39        (stage 2 live, room 1, hundreds of frames presented)
resource markers (distinct names in markers.jsonl):
   1s1.pcx  1s2.pcx  1e1.pcx  stage1.pcx  stage1.stg  stage1.wal  night1.chr  help8.pcx  levelup2.pcx
   NO 2s1.pcx  NO 2s2.pcx  NO stage2.pcx  NO stage2.stg  NO stage2.wal  NO night2.chr
```

So docs/224's finding **still holds on HEAD** (after item 4's `fn_00035d5d` card-wait fix ✓ docs/231 ✓): the port
advances `g_dat_63370` to 2 through the game's own room-clear tail, runs the end slides (`1e1.pcx`) and the level-up
slideshow (`levelup2.pcx`), and then keeps running on **stage-1 assets** ✓ — `stage1.wal`, `stage1.stg`, `night1.chr`
are the only room/art resources in the whole run ✓ That is the observable content of item 5's "floor renders black":
the stage-2 room is composited (or not composited) from stage-1 art ✓

## 2. What is *not* the cause (all verified this step)

* **The stage-advance tail is byte-faithful.** Shipped `0x3b520 inc [0x13370]` → `0x3b526 call 0x10dde` → `0x3b52b
  call 0x405a7` → `0x3b535 [player+0x30] == 1 ? 0x3b54c : (0x3b542 mov eax,1; call 0x38f3d)` → `0x3b54c call 0x374ad`
  → `0x3b551 jmp 0x3b58d → 0x3b58d jmp 0x3b05a` ✓ == port `f.c:28058-28063`: `g_dat_63370 += 1; fn_00010dde();
  fn_000405a7(); if (g_player_ent[0x30] != 1) fn_038f3d(1); fn_0374ad(); goto room_start;` ✓✓ (docs/224 §4's
  "0x0003b52b" for the increment is off by six bytes — the increment is `0x3b520`, `0x3b52b` is the `call 0x10dde` ✓)
* **`fn_000405a7` is not a stage loader** ✓ — it is 54 B of DPMI real-mode call `{0x13}` (`0x405bf mov word
  [ebp-0x1c], 0x13; call 0x41e51` ✓), modelled as `sys_cd_audio_stop()` ✓, with 7 shipped call sites
  (`0x10caf, 0x365aa, 0x38af8, 0x3a57d, 0x3b1ad, 0x3b3ec, 0x3b52b` ✓) and 15 mentions in `f.c` ✓
* **`fn_0374ad` (the stage reset/reload) is modelled including its reload**: shipped `0x37615 call 0x37b03` ✓ ==
  `f.c` `fn_037b03(0);  /* 0x37615 full reload */` ✓, plus `0x37629 call 0x3cad3` ✓, `0x3763b mov [0x13398], eax` ✓
* **The slide-count tables are byte-identical to shipped memory.** Read out of a shipped DS-window sample in `rm1`:
  `DAT_00052f2c = [22,3,2,6,0,0,3,0,1,3]` ✓ vs `g_tab_52f2c = {0x16,3,2,6,0,0,3,0,1,3}` ✓ (`f.c:27848`) and
  `DAT_00052f90 = [0,1,1,2,0,4,3,1,1,3]` ✓ vs `g_tab_52f90` ✓ (`f.c:27849`) ✓ — so `g_tab_52f2c[2] = 2 ≠ 0` ✓: the
  stage-2 start slides **should** run and should request `2s1.pcx`/`2s2.pcx` ✓, and `stage%d.pcx` (stage 2) is
  requested in the same block (`f.c:27998-28001` ✓ == shipped `0x3b14a` ✓) ✓ yet none of those lookups happen ✓

## 3. The remaining open question, stated narrowly

`room_start` (`f.c:27971` ✓ == `LAB_0003b05a` ✓) wraps its slides **and** the `stage%d.pcx` lookup in
`if (fn_03f15f() == 1)` ✓ (shipped `0x3b061 call 0x3f15f` ✓, `0x3b066 cmp eax,1` ✓). docs/268 measured
`fn_03f15f()` returning 1 on the *warp* route ✓ — so the live question is **whether the port reaches `room_start` at
all after the stage-2 transition, and if it does, what `fn_03f15f()` returns there** ✓ Next measurement (bounded,
one witness): log at `room_start` entry — `g_dat_63370`, `fn_03f15f()`'s return, `g_tab_52f2c[stage]`, and the two
`fn_0428f8` name renders — on the natural `ep1_roomclear2` route ✓ If `room_start` is not re-entered, the fault is in
the tail's control flow (`goto room_start` vs the shipped's `jmp 0x3b05a` path, and `fn_0374ad`'s internal branches ✓);
if it is re-entered with `fn_03f15f() != 1`, the fault is in the port's VESA-shim state at that moment ✓ (its body
`f.c:16090` ✓ is faithful to `0x3f15f-0x3f216` ✓, so a wrong return there is a host-shim state issue, not a
transcription error ✓)

## 4. Rig hygiene note (my own error this step)

I parsed `frames.bin` with a hand-written `struct.Struct("<IIHHI")` and got garbage records (`mode (50679,50584)
t=1571389.9s changed=949536152` ✗✗) which flooded the log ✗ The sanctioned parser is `scripts/rig/diff_screen.py::
read_frames` (its own `HDR`/`REC` layout ✓, frames are `{t, frame, w, h, changed, sig}` dicts with an **8-byte
signature**, not pixel data ✗ — full pixel bins exist only for the saved *witness* frames, and only 145 of them in
`rm1/orig_screen/frames` ✓ covering the first ~4.5 s ✗). Rule recorded: **mode/segment censuses must come from
`read_frames`, and per-pixel region censuses are only possible on saved witnesses** ✓

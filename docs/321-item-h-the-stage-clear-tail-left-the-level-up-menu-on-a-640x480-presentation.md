# 321. Item H: the stage-clear tail left the level-up menu on a 640x480 presentation, because the shipped's mode-13h call was modelled as an audio stop

Round-4 item H (2026-11-03 playtest): "after clearing stage 3 a single picture is shown, then the
upgrade menu opens, and the screen stays glitched until the menu is exited."

## 0. The shipped's tail, in order (capstone over NIGHT.EXE)

`fn_03af18`'s stage-clear tail, reached only when `fn_03a829` returns 1:

| offset | instruction | what it does |
|---|---|---|
| 0x3b1b2 / 0x3b1b7 | `call 0x3a829` / `mov [ebp-0x44], eax` | the room routine's return value (its only call site) |
| 0x3b1bf | `cmp eax, 1 / jne 0x3b553` | **only a return of 1 reaches the stage tail**; everything else goes to the CONTINUE path (`cmp [ebp-0x44], 2` at 0x3b553 -> `call 0x376c8` at 0x3b559) |
| 0x3b1c9 | `call 0x3ace9` | stage-clear bookkeeping |
| 0x3b438-0x3b51e | end-slide loop | `fn_03628e` (the 640x480 window loader, 0x3b4c8) + `fn_0406f5`/`fn_040660` fades + `call 0x35dfc` (the card key wait, 0x3b4dc), gated on `g_tab_52f90[stage]` (`cmp eax, [edx+0x2f90] / jl 0x3b4a6` at 0x3b518); stage 3's count is 2, stage 2's is 1 |
| 0x3b520 | `inc dword [0x13370]` | the stage counter advances |
| 0x3b526 | `call 0x10dde` | |
| **0x3b52b** | **`call 0x405a7`** | **the shipped puts the screen back into VGA mode 13h (320x200x256)** |
| 0x3b530-0x3b540 | `mov al, [player+0x30] / cmp eax,1 / je 0x3b54c` | skip the menu if the player record is dying |
| 0x3b542 / 0x3b547 | `mov eax, 1` / `call 0x38f3d` | the upgrade menu (variant 1, `levelup2.pcx`) |
| 0x3b54c | `call 0x374ad` | the stage reset (`fn_037b03(0)` at its 0x37615, then `DAT_63354 = DAT_6334c`, `fn_03cad3` at 0x37629) |

`fn_000405a7` is not an audio call.  Its body:

```
0x405bf: mov  word ptr [ebp - 0x1c], 0x13      ; record dword 0 = 0x13
0x405c5: lea  ebx, [ebp - 0x1c]
0x405c8: lea  edx, [ebp - 0x1c]
0x405cb: mov  eax, 0x10                        ; the interrupt selector
0x405d0: call 0x41e51
```

and `fn_041e51 -> fn_041e38 -> fn_043324 -> fn_04335c` loads the register set out of that record
(0x4336d-0x4337b: `mov eax,[edi] / mov ebx,[edi+4] / mov ecx,[edi+8] / mov edx,[edi+0xc] /
mov esi,[edi+0x10] / mov edi,[edi+0x14]`), so the record is a register set and its dword 0 is **AX =
0x13 = mode 13h**.  Its sibling `fn_00040570` is the identical shape with **AX = 3** (text mode 3 =
720x400) and is called from the title-abort path `fn_037306` at 0x37331 and from `fn_036537` at 0x36552
— and the shipped really does sit in 720x400 text at the title (docs/46 measured it).  Every one of the
seven call sites is a "put the screen back to 320x200" moment: 0x10caf (after the `cdinsert.pcx` key
wait), 0x365aa (`fn_036570`, room bringup), 0x38af8 (before the track-1 select at 0x38afd), 0x3a57d (the
tail of the CG picture show `fn_03a70e`), and 0x3b1ad / 0x3b52b / one more inside `fn_03af18`.

The shipped's own frame dumps confirm the mode bookkeeping: `/tmp/rig/rb320/orig_screen/frames.bin`
(identical in every cached shipped half — `rb13`, `rb15`, `rb16`, `rb17`, `rb320`) flips

```
640x480 t9..46338 | 320x200 t46338..47699 | 640x480 t47699..62802 | 320x200 t62802..62903 |
640x480 t62903..73900 | 320x200 t73900..76357
```

— cards in 640x480, rooms in 320x200, and it returns to 320x200 after every card show ✓.

## 1. What the port did instead

`fn_000405a7` was modelled as `sys_cd_audio_stop()` — a host hook, no geometry effect (`f.c` before this
fix, "the MSCDEX 'stop audio' real-mode request").  The port's presentation geometry is
`main.c:168` `vw = g_view_320x200 ? 320 : sys_video_w()`, where `g_view_320x200` is a composite hint set
by the room path (`f.c:15774`, `f.c:15999`) and cleared when a copy fills a full 640x480 frame
(`f.c:8199 if (count >= 0x1e0u) g_view_320x200 = 0`).  So after the tail's end slides the hint is 0 and
nothing in the tail restores `sys_video_w/h` to 320x200: the level-up menu then presents its 320x200
`levelup2.pcx` plane (`0x39762-0x39767`, `mem_copy` of 0xfa00 from the picture buffer into `DAT_633b0`,
and the present at 0x399ff) into a 640x480 window — 64000 bytes cover only the first 100 of 480 rows,
the rest of the window keeps the previous slide's bytes.  That is the reported garble, and it persists
for exactly as long as the menu keeps re-deriving its backdrop from that buffer — i.e. until the menu is
exited ✓ matching the report.

## 2. Red / green, same drive, same pinned env

Drive `scripts/rig/drives/ep1_st3clear_h.txt` (arm 3 only: `DAT_6336c = 3` is the one request that makes
`fn_03a829` return 1 — 0x3abca-0x3abdd — and in the game it is written by `fn_00012dcc` at 0x12a04 when
the record being removed equals the latch `DAT_00053c80`, 0x129fc `cmp eax,[0x3c80]`; arms 2 and 4 only
advance rooms).  Env `NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60 NIGHT_SKIP_INTRO=1
NIGHT_ROOMSTART=1 NIGHT_REQUESTDBG=1 NIGHT_ROOMDBG=1 NIGHT_MENUBG=1`, basis `--warp-episode 3`
(`DAT_63370 = 3`, the shipped's own stage counter, docs/317).  Every claim here is an arm-3-poke claim.

| | red `/tmp/rig/h321d` (HEAD `8fd5c2a`) | green `/tmp/rig/h321e` (this fix) |
|---|---|---|
| request consumed | `[request] seen tick=5699 value=3 iter=1 room=0x18d6ec stage=3` | `[request] seen tick=7220 value=3 iter=1` |
| tail ran | `roomloop: r=1`, `tail: stage=4` | `tail: stage=4` |
| menu opened | `fn_038f3d allocs … pcx=0x337cf0`, `fn_036190: levelup2.pcx`; `DAT_633a4` 0->1 at t=83000 | same, `DAT_633a4` 0->1 at t=92000; stage 3->4 at t=91500 |
| **presentation geometry over the menu** | `640x480 t2000..58316 (6757 f)`, `320x200 t58316..68783 (1257 f)`, **`640x480 t68783..135333 (9359 f)`** — the menu window (t=83000) sits inside a 640x480 segment, and nothing returns to 320x200 for the rest of the run | `640x480 t2000..63816`, `320x200 t63816..87833`, `640x480 t87833..91083` (the end slides), **`320x200 t91083..135333 (6651 f)`** — the menu window (t=92000) sits inside a 320x200 segment |
| mode log order | `display mode 640x480` at 08:35:34 then the menu at 08:35:48, no `320x200` line | `display mode 640x480` (end slides) -> `tail: stage=4` -> `levelup2.pcx` -> **`display mode 320x200`** |

Geometry is read from each side's `frames.bin` record header through `scripts/rig/diff_screen.py`'s
`read_frames` (the shared reader, not a hand-rolled one).  This is a *what*-claim: the two sides' geometry
segments, not any pacing or latency statement (docs/234 §3).

## 3. The fix, and what it is not

`f.c`: `void fn_000405a7(void) { sys_vga_set_mode(0x13); sys_cd_audio_stop(); }` with the shipped's
instruction offsets in the header comment; `sys.c`/`sys.h`: a new seam `sys_vga_set_mode(al)` mapping
0x13 -> 320x200 (the geometry variables are the same ones `sys_vesa_set_mode` already drives, so the VBE
0x101 card-show sets and the mode-13h restores now alternate the way the shipped's dumps show them).
The named offset whose behaviour changed is **0x3b52b** (and the six other `call 0x405a7` sites, which
are now the mode restores the shipped intends).

Kept, and labelled: `sys_cd_audio_stop()`.  The earlier reading that this routine *is* the shipped's CD
audio stop (docs/06, docs/28, docs/31, docs/105, docs/106) is superseded by the register record above;
the hook stays because the port's CD bookkeeping and two fixtures count it, and whether the shipped
stops CD-DA at these seven sites cannot be measured on this basis (docs/304: the shipped half carries no
CD-DA at all).  Recording it as an open item rather than deleting a measured-green witness.

Not modelled: `fn_00040570`'s AX = 3 (text mode 3 = 720x400, sites 0x36552 and 0x37331).  Wiring it
would change the title-phase window geometry, which the rig has no shipped-side card evidence for on
skip-intro drives, so it is left as an open modelling item.

## 4. Also measured this turn, on the same basis

- `g_tab_52f90 = {0,1,1,2,0,4,3,1,1,3}` (`f.c:28470`) — stage 2's end-slide count is 1, stage 3's is 2.
  The playtester's "a single picture" fits a stage-2 tail exactly, and fits a stage-3 tail if a press
  ends the slide loop early (`fn_00035dfc`'s return gates the loop at 0x3b4e1).  Not needed for the
  verdict: the mode ordering is the same in both cases.
- Drive lesson: a request written before `fn_03a829`'s per-room frame loop exists is never consumed —
  in `/tmp/rig/h321b` `DAT_6336c` was written at t~37 s and stayed 3 for the rest of the run, because
  the request is cleared at routine *entry* (0x3a95a / 0x3a9e1) and tested only at the bottom of the
  loop (0x3abae onward).

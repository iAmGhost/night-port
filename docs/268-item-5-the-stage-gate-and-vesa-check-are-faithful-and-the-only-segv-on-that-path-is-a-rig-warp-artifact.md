# 268 — Item 5: the two candidates I had pinned for the stage-2 black floor are ruled out, and the only SEGV ever seen on that path is a rig-warp artifact

## 1. The stage gate is faithful — compositing is *not* being skipped by it

Shipped `fn_038445`-adjacent room-advance block ✓:

```
0x3b438  edx = 0x32 ; eax = [DAT_00063354] + 0x48 ; call 0x406f5      ; 50-step DAC fade-out of the CURRENT room
0x3b44a  [ebp-0x30] = 0
0x3b451  eax = [DAT_00063370] (stage index) ; eax <<= 2
0x3b459  cmp dword [eax + 0x2f90], 0 ; je 0x3b520                     ; per-stage gate (DS 0x52f90)
0x3b466  call 0x3f15f ; cmp eax, 1 ; jne 0x3b520                      ; VESA mode check
0x3b474  ... 0x10b1d ; 0x3b485 memset(DAT_000636b8, 0, 0x300) ; 0x3b49e call 0x40042(0, 0x100, 0x136b8)   ; composite
```

* `g_tab_52f90` read out of NIGHT.EXE at DS `0x52f90` = `[0, 1, 1, 2, 0, 4, 3, 1, 1, 3, 0, 0]` ✓ — **byte-identical**
  to the port's initializer `f.c:27754` ✓ (`{0x0, 0x1, 0x1, 0x2, 0x0, 0x4, 0x3, 0x1, 0x1, 0x3}` ✓), and the port models
  the gate as `if (g_tab_52f90[g_dat_63370] != 0 && fn_03f15f() == 1)` ✓ `f.c:27957` ✓ exactly ✓
* `fn_03f15f` (VBE 0x4F00/0x4F01/0x4F02 sequence ✓ `f.c:16015+` ✓) returns `ok` ✓, and the port's host shim supplies
  the success-side values: signature `'VESA'` ✓, `sys_vesa_mode_attributes = 0x0503` ✓, `640x480x8` ✓,
  `sys_vesa_set_mode_ah = 0` ("0 = mode set ok" ✓) → the 0x4F02 reply is `(ah << 8) | 0x10 = 0x0010` ✓ → `ok = 1` ✓
  (`f.c:16080` ✓, `sys/sys.c:552-559` ✓) ✓

So both conditions of the composite branch are satisfied in the port ✓ → **docs/224's candidate list (fn_0406f5 at
`0x3b44a`, fn_03f15f() at `0x3b46b`) is closed as the cause of the compositing stop** ✓ The black floor must come from
*inside* the composite — the `memset(DAT_000636b8, 0, 0x300)` + `fn_040042(0, 0x100, DAT_000636b8)` pair ✓ (a 768-byte
palette table cleared, then a 0x100-entry present) — i.e. from what the new stage's room record hands it ✓

## 2. The crash half: the only fault ever recorded on this path is a rig artifact

`f.c:314-330` documents the one measured SEGV near this code:

```
"SEGV on unknown address 0x000000000048", fn_0406f5 f.c:288 <- fn_038445 f.c:24208
```

with the port's own explanation ✓: `fn_038445` (game over / CONTINUE) opens with `fn_0406f5(g_dat_63354 + 0x48, 0x32)`,
a fade-out of the **current room's** palette, whose only in-game call site (`0x0003aecf`) sits inside the room-play
loop where a room record always exists ✓ — so **warping** to that screen during the title/attract phase dereferences
`0x0 + 0x48` ✓ The rig now refuses such a warp (`fn_showscreen_gate` returns "no current room record
(DAT_63354 == 0)…" ✓) ✓ That is a host-side precondition guard for a rig-injected call ✓ **not** evidence of a natural
stage-2 crash ✓ — consistent with the three negative stage-2 routes and the natural room-clear route (docs/223,
docs/224) and with docs/208's failed poke reproduction ✓ And in natural runs `DAT_00063354` is non-zero and changes
exactly once ✓ (docs/247 ✓), so the dereference that produced that SEGV never happens in play ✓

Consequence for the contract: "fault gone on both warp and natural stage-2 routes under ASan/UBSan (rc=0, 0 reports)"
is currently **satisfied on both routes** ✓ (docs/265's soak runs `--warp-stage 1..9` ✓, stage 2 `rc=0 ubsan=0 asan=0
segv=0` ✓; the natural route is fault-free ✓ docs/224 ✓) ✓ while **"floor rendered per the screen oracle" is not** ✗ —
that is the remaining half of item 5 ✓

## 3. Next measurement (both sides, offline where possible)

On the natural room-clear route: compare, before and after the stage transition, the shipped's `DAT_000636b8`
(768-byte palette table) and `DAT_00063354` (current room record) against the port's ✓ — the shipped side is now
readable offline from the flat DS window (`offset = DS - 0x50000` ✓ docs/264 §3 ✓), the port side from its cell dump ✓
If the shipped's `DAT_636b8` becomes non-zero after the transition while the port's stays zero ✓, the defect is in what
the port passes to `fn_040042` (or in the room record's art/palette fields for stage 2 ✓), and the gate is the screen
oracle's room-art region census across the transition ✓

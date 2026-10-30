# 216 — RETRACTED (see docs/218 §2): the copy direction here was read backwards

> **This document's central claim is wrong and superseded by docs/218 §2.**  cdecl pushes arguments
> right-to-left, so `push 0xa0000` / `push 0x3e80` / `push [ebp-4]` at guest 0x00038154-0x00038168 passes
> arg1 = `[ebp-4]` (source, the composite buffer), arg2 = 0x3e80, arg3 = 0xa0000 (destination, the video plane) --
> and the callee body confirms the slots (`edi = [ebp+0x10]` destination, `esi = [ebp+8]` source,
> `rep movsd es:[edi], [esi]`).  The shipped therefore presents the composite into the video plane, exactly as the
> port does.  The instruction sequence, the caller evidence (`0x0003ab27 mov eax,[0x133b0]; call 0x38098`) and the
> blit census below are accurate; the direction -- and every consequence drawn from it -- is withdrawn.  It is kept
> because "a fix based on this would have inverted a working copy" is a fact worth recording.


## 1. The shipped instruction sequence and what it means

`fn_038098` (the room pump composite) ends its draw block with, at guest 0x00038154-0x00038168:

```
0x38154  push 0xa0000            ; a  = 0xa0000            (the video plane)
0x38159  mov eax, 0x3e80  push   ; n  = 0x3e80 dwords      (16 000 dwords = 64 000 B = one 320x200 plane)
0x3815f  mov eax, [ebp-4]  push  ; c  = the composite buffer (DAT_000633b0, the back screen)
0x38163  call 0x404c6
0x38168  add esp, 0xc
```

`docs/12-functions/primitives.md` fixes the callee's semantics from its own body
(`for (; n; n--) { *c = *a; a++; c++; }`, REP MOVSD): **a = src, n = dword count, c = dst**.
So the shipped moves pixels **from the video plane into the back-screen buffer** -- it reseeds the composite
buffer from what the screen currently holds.  The base is the back screen because the pump is entered as
`fn_038098(g_dat_633b0)` (`f.c:23372`, mirroring guest 0x0003ab31), and `g_dat_633b0 = SCREEN_BASE` is the port's
back-screen arena (`f.c:7426`).

## 2. What the port does instead

```c
cpy_dw((uint32_t *)video_map(base), 0x3e80, (uint32_t *)g_video_ram);   /* 0x38154-68, f.c:15388 */
```

`video_map(base)` with `base == SCREEN_BASE` maps to `g_screen_ram` (the back screen, `f.c:7429-7434`), so the port's
call is `src = back screen, dst = g_video_ram`: **the opposite direction**.  The runtime census agrees, and shows how
hot the site is (`/tmp/rig/bs1`, `NIGHT_BLITSTATS=1`, drive `ep1_kill3`, 8 000 frames):

```
== blit stats (dst: 0=other 1=back screen 2=video arena) ==
  fn_03ce0b   calls=5960  bytes=381440000  dst=1     (the room plane, into the back screen)
  fn_03801b   calls=   1  bytes=     64000  dst=2
  fn_0381d7   calls= 120  bytes=  7680000  dst=2     (the walk-step composite, same reversed copy)
  fn_038098   calls=5839  bytes=373696000  dst=2     <-- every room frame overwrites the video arena
```

So on the current build the port writes the video arena from the back screen 5 839 times, while the shipped reads
the arena and writes it into the back screen.  `fn_0381d7` (the walk-step composite, whose shipped tail is the same
`push 0xa0000 / push 0x3e80 / push base / call 0x404c6`) shows the same reversed direction.

## 3. Why this is the strongest candidate for item 5's black floor, and what is still unproven

Everything the composite draws goes into the back screen (`fn_03203d`, `fn_03df96`, `fn_03b953`, `fn_0137c1` all
receive `base`), and in the port the *arena* -- the buffer whose contents the host presents, and the one
`fn_03f15f`'s VBE 0x101 640x480 linear model writes (docs/205) -- is overwritten with those back-screen bytes once
per room frame.  In the shipped, the arena is never overwritten here; the back screen is refreshed *from* it.  A
buffer the port overwrites and the shipped preserves is exactly the buffer whose failure to hold the room's floor
would show up as a black floor, and it is also the buffer that decides whether a floating kill number is presented
(docs/215 §4: the shipped's presented frame carries no popup ink at the popup's own coordinates).

Unproven, and deliberately not claimed here: (a) that this reversal *alone* produces item 5's black floor -- the
stage-2 route in docs/208 runs 13 435 frames without a fault on either side, so the crash half of item 5 still has
no rig reproduction and no ASan/UBSan fault attribution; (b) which shipped site presents the composite in the
shipped build (the candidate is `fn_03ace9`'s `mem_copy(A0000 <- 633b4, 0xfa00)` at guest 0x0003add5, docs/110).  Both
are gate work, not fix work: this document records the divergence, its bytes, and its census, on the unfixed build.

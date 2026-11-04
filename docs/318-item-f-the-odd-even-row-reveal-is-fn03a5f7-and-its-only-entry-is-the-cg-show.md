# 318. Item F: the odd/even row-offset reveal is fn_03a5f7, its only entry is the CG show, and the "0 offset arms" census was a warp-basis artifact

Goal task-10 (round-4 item F: "the effect the original plays at the actual start of stage 3, after the dialogue —
odd/even screen rows offset horizontally across the screen — is missing in the port").

## 1. The effect's writer, named

`fn_03a5f7` (0x3a5f7-0x3a708, 278 B) walks 200 screen rows and offsets them horizontally by an accumulating
column counter:

```
0x3a612  mov [ebp-8], 0                      ; colAcc
0x3a619  mov [0x13e24], 0                    ; DAT_63e24 (the tick gate)
0x3a623  mov eax,0x1ea / idiv [0x3104]       ; n = 490 / DAT_53104
0x3a636  cmp eax,[0x13e24] / jle 0x3a6c0
0x3a64b  for (row=0; row<0xc8; ...)                          ; 200 rows
0x3a65a  mov ebx,2 ; 0x3a665 sar/idiv ebx ; 0x3a66a test edx,edx / 0x3a66c je 0x3a698
           -> the PARITY test on the row index (row % 2)
0x3a66e-0x3a691   ODD : src = base + (row+1)*0x140 - colAcc - 1
                    dst = 0xa0000 + row*0x140              ; CALL 0x41e6c
0x3a698-0x3a6b9   EVEN: dst = 0xa0000 + (row+1)*0x140 - colAcc - 1
                    src = base + row*0x140                 ; CALL 0x41e6c
0x3a6ee  sub [0x13e24], n ; 0x3a6f4 colAcc += 4 ; 0x3a6f8 cmp 0x140 / jl loop
```

Every row is copied with a *different horizontal shift*, the shift growing by 4 per pass until it reaches 0x140 —
that is exactly "odd/even rows offset horizontally across the screen", and it is a screen-wide tear, not a sprite.

`scripts/audit/call_xrefs.py` (4503 resolved direct calls): **`fn_03a5f7` has one call site in the entire binary —
0x3a804, inside `fn_03a70e`** (the CG picture show's tail, right after the last `na%d%04d.pcx` card).

## 2. The port has it, and a fixture pins its arithmetic

`src/f/f.c:14462` models it (`fn_03a5f7(const uint8_t *base)` — the parity split, `colAcc += 4`, the
`0xa0000 + row*0x140` destinations and the `colAcc + 1` counts), called from the port's `fn_03a70e` at
`f.c:16171` (0x3a7e7-f0 ✓).  `test/test_f1.c:12153 test_fn_03a5f7` pins the offsets row by row:

```
T1 (g_dat_53104=0x46 -> n=7, 80 passes, last colAcc=316):
  row0 (even) scr[3..319]   = b[0..316]      scr[3,13,203,319] = 00,0a,c8,3c
  row1 (odd)  scr[320..636] = b[323..639]    scr[320,330,636]  = 43,4d,7f
  row2 (even) scr[643..959] = b[640..956]
  gap bytes scr[0,1,2] and scr[637,638,639] never written
  g_dat_63e24 = 0 - 80*7 = -560
T2 (n=0 gate): no writes at all, DAT_63e24 stays 0
```

So the "green" half of the contract — offsets matching the shipped's arithmetic for the same records — is held by
a fixture against the shipped's own instruction offsets, and `make test` is ALL PASS with it in the run.

## 3. The "missing offset arms on a stage-3 basis" census was a basis artifact

The number that looked like the defect was `fn_03ce0b`'s arm census.  Its arm selector is one byte, from bytes:

```
0x3ce2a  mov al, [0x139e4]        ; DAT_639e4, the vertical-scroll mode byte
0x3ce2f  test eax,eax
0x3ce31  jne 0x3ce70              ; != 0 -> the vertical/partial-row arms
         ; == 0 -> 0x3ce33-0x3ce63: CALL 0x40491 with x = [0x139dc](camX) + [room+0x348] - [room+0x00], then 0x3d071
0x3ce70  (vertical arm) 0x3ce79 cmp [0x139e0](camY), [room+4] / 0x3ce7c jge 0x3cf56
```

Measured with `NIGHT_BLITDBG=1 NIGHT_BLITEDGE=1`:

| basis | run | blt_rows calls | arms | out-of-band | row spans inspected |
|---|---|---|---|---|---|
| `--warp-episode 3` (shipped stage counter = 3, warp install) | `/tmp/rig/f317a`, 115 samples at `memstat stage=3 room=1` | 1454 | **B0 only** (B1=B2=B3=0) | 0 | 868,007, all inside one row, 0 crossings |
| legitimate episode-1 chain, `ep1_walk_st2.txt`, advance to the third node | `/tmp/rig/f317b`, `DAT_63354` 0x18d6ec → 0x18da60 (32.0 s) → **0x18ddd4** (53.5 s) | 2297 | **B0=1956, B2=125, B3=91** | 0 | 5,469,145, 0 crossings |

The warp run's `DAT_639e4` never leaves 0, because a warped node is never handed to the camera routine
(docs/312 §0a: every `[vgate]` visit on a warp-stage basis reports `node=0`, the chain head ✓) — so its arm
selector stays on the horizontal arm by construction.  On the chain basis, with item D's vertical follow firing
(`DAT_639e0`: 0 → 24 (51.5 s) → 71 → 119 → 166 → 200 (53.5 s) ✓ `DAT_639e4`: 0 → 1 at 49.0 s ✓ `fn_01586b` band
following camY: 115/195 → 116/196 → 117/197 → 118/198 ✓ player DOWN calls 1511 ✓), the partial-row arms fire
125 + 91 times.  Same build, same witnesses, different basis: the difference is the drive, not the code.

## 4. What is actually missing at stage 3, and why it is the same blocker as item B

`fn_03a5f7` is reachable only through `fn_03a70e`, and `fn_03a70e` is armed only by `fn_03a829`'s band test on
`DAT_00053c80` (0x3aa3b-0x3aa9a), which `fn_010efc` fills only when a record whose **type index equals the stage
counter** is constructed (0x10f1d-0x10f26).  On the stage-3 basis (`NIGHT_STGDBG=1`, `/tmp/rig/e318a`):

```
stgdbg stage=3 records=128 tag1: 3:1 71:23 72:13 76:19 77:6 81:6 83:8
stgboss stage=3 rec=0030c71c sub=1 x=6403 y=186 zone=0 posmode=1
latch: none   na*.pcx: 0   show: none     (DAT_63370 = 3 proven by the constructed types 71/72/76)
```

Stage 3 declares exactly one type-3 record — the boss the latch needs — at `x=6403 y=186`, and in the sampled
window no record of that type is ever constructed, so the latch stays null, the CG show never runs, and its
reveal tear never plays.  The shipped's own census on the canonical 150 s episode-1 route
(`/tmp/rig/rb15/orig/state_*.ds.bin`, 412 samples) holds `DAT_00053c80 = 0` in 412/412 samples, matching the
port's 1500/1500 on the same drive — so the *latch being empty in stage 1's early window* is agreement, not
divergence, and the stage-3 question is a spawn-window question (docs/312 §2: the boss record exists and is
window-rejected).

## 5. Verdict and what is owed

* **No defect found in `fn_03ce0b` or `fn_03a5f7`.**  The reveal arithmetic is transcribed and fixture-pinned; the
  arm mix that looked missing is a warp-basis artifact, and the legitimate-basis census shows the partial-row
  arms running.
* Item F as reported (an effect at stage 3 that the port never shows) is **not reproduced by any rig drive yet**:
  the only routine that produces it is gated behind the CG show, which no drive has armed.  Recorded as
  not-reproducible, not as "no defect".
* Owed from the playtester: the stage-3 run itself — did the tear appear *immediately after* the six-card opening
  (i.e. before the first room frame), and how many pictures preceded it?  If it played after a picture strip, the
  strip is the CG show and the fix belongs to the stage-3 spawn window (the type-3 record's admission), which is
  the same gate that produced round-2 item B.  If it played with no picture strip at all, then some routine other
  than `fn_03a5f7` draws it — and `fn_03a5f7`'s single call site means that routine is a different one, which the
  rig has not identified.
* Standing rule this item adds: **an arm/dispatch census is only quotable on a basis that can set the selector
  byte.**  A census of "which arms ran" on a warp basis measures the warp, not the game.

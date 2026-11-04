# docs/311 — fn_03ce0b: four row/column axis errors and two unsigned arm tests, found from the shipped bytes and witnessed live

Fix commit: `9abb102` ("fn_03ce0b: four row/column axis errors and two unsigned arm tests, found from the
shipped bytes and witnessed live").  Landed for the 2026-11-03 playtest, round 1 (the crash report).

## 1. What the playtester produced

The 2026-11-03 session (`docs/manual-playtest/05-2026-11-03.md`, round 1) ended in a crash on entering stage
2, and this time the report came with a fault log (`/tmp/play_asan.log`):

```
ERROR: AddressSanitizer: ... on address ... WRITE of size ...
  #1 blt_rows           src/f/f.c:115
  #2 fn_03ce0b
  #3 fn_038098
  #4 fn_03a829
  #5 fn_03af18
```

That is the first playtest fault whose stack points at a *row blit*, so the question was not "add a guard"
but "which arithmetic produced a row count the shipped never computes".

## 2. Why a bad row count is catastrophic rather than harmless

`blt_rows` (f.c:107, shipped `0x040491`) copies `h` rows of `w` bytes with `src += s - w`, `dst += t - w` per
row.  The shipped's row loop counts a dword down and jumps while non-zero, so `h = 0` or a negative `h` is not
a no-op: it iterates ~4e9 times and writes past the destination.  The port is faithful to that loop, so the
loop itself is not the defect and adding a guard inside it would hide the caller's error.

Methodology note, stated honestly: the loop shape comes from the Ghidra listing the port was transcribed
from.  A capstone pass over `0x04080-0x040d0` in this turn decoded as data (`add byte ptr [eax], al` runs),
so the DEC/JNE offsets were not re-verified from raw bytes here; every fn_03ce0b offset quoted below *was*
re-read from capstone output this turn.

## 3. The witness that made it visible (NIGHT_BLITDBG=1)

Two witnesses, both under `NIGHT_BLITDBG=1`:

- `blt_rows` (f.c:108-121) reports any call whose row count leaves the shipped's `1..0xc8` band, naming the
  width, pitches, `rows_bytes` and the caller address.  Counter `g_blit_bad_reported` (f.c:90).
- `fn_03ce0b` (f.c:7726-7732, 7745-7750) logs its arm dispatch — `B0/B1/B2/B3` — with `mode`, the scroll row
  `g0`, `e[0x00]`, `e[0x04]`, `e[0x0c]`, `e[0x1a]`.  Counter array `g_ce0b_arms[4]` (f.c:91).

The arm witness exists because the row arithmetic is arm-specific: logging only "a row count left the band"
would not have identified which of the four arms' three different `* 0x140` products was wrong.

## 4. Red, witnessed

`/tmp/rig/g311a` (unfixed build, pinned env `NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`,
skip-intro, `NIGHT_BLITDBG=1`), two `blitdbg` lines in the whole run:

```
blitdbg: fn_03ce0b arm=B2 mode=1 g0=1 e[0]=3280 e[4]=0 e[0xc]=200 e[0x1a]=320
blitdbg: blt_rows h=-3279 w=0x140 src=0x1004d4b30 dst=0x1042db8c0 spitch=1920 dpitch=320
         rows_bytes=-1049280 from=0x10018daac
```

`h = -3279` is `g0 - e[0x00]` = `1 - 3280`: the arm had taken the record's **X** field.  A negative row count
reproduces the playtester's fault on the host.

## 5. The shipped bytes (capstone over NIGHT.EXE, re-read this turn)

Arm tests — both signed, which is what selects the arm for a record with its sign bit set:

```
0x3ce70: mov  edx, [0x139e0]        ; DAT_000639e0 = scroll row (g0)
0x3ce76: mov  eax, [ebp-8]          ; entity record
0x3ce79: cmp  edx, [eax+4]          ; compares against the record's TOP Y field, not +0x00
0x3ce7c: jge  0x3cf56               ; signed: B1 runs only while g0 < e[0x04]
...
0x3cf56: mov  edx, [0x139e0]
0x3cf5c: add  edx, 0xc8
0x3cf62: mov  eax, [ebp-8]
0x3cf65: cmp  edx, [eax+0xc]
0x3cf68: jle  0x3d030               ; signed: B2 runs only while g0 + 0xc8 > e[0x0c]
```

The three products that multiply a **row** delta by the row pitch `0x140`:

```
0x3cfa0: mov  eax, [0x139e0]
0x3cfa5: mov  edx, [ebp-8]
0x3cfa8: sub  eax, [edx+4]          ; g0 - e[0x04]   (the port had g0 - e[0x00])
0x3cfab: imul edx, eax, 0x140
0x3cfb1: mov  eax, [ebp-8]
0x3cfb4: mov  eax, [eax+0x348]
0x3cfba: add  eax, edx              ; a = e[0x348] + (g0 - e[0x04]) * 0x140
0x3cfbc: push eax
```

plus, in the same arm, `h = g0 - e[0x04]` (`0x3cfe5-0x3cff0`) and
`c = dst + ((0xc8 - g0) + e[0x04]) * 0x140` (`0x3cfca-0x3cfe4`), and in arm B3
`a = e[0x348] + (g0 - e[0x04]) * e[0x1a]` (`0x3d049-0x3d066`).

`e[0x00]` is the record's column; `e[0x04]` is its top row.  Every one of these four products is a row count
or a row-pitch multiple, so all four must read `+0x04`.

## 6. The fix (f.c after `9abb102`)

| site | port line | was | now |
| --- | --- | --- | --- |
| B2 first call, `a` | f.c:7756-7757 | `g0 - e[0x00]` | `g0 - e[0x04]` (0x3cfa8) |
| B2 second call, `h` | f.c:7767 | `g0 - e[0x00]` | `g0 - e[0x04]` (0x3cfe5-0x3cff0) |
| B2 second call, `c` | f.c:7770 | `(0xc8 - g0) + e[0x00]` | `(0xc8 - g0) + e[0x04]` (0x3cfca-0x3cfe4) |
| B2 second call, `c` scale | f.c:7770 | missing `* 0x140` term | `* 0x140` as the shipped computes it |
| B3, `a` | f.c:7777-7778 | `g0 - e[0x00]` | `g0 - e[0x04]` (0x3d049-0x3d066) |
| B1 arm test | f.c:7726 | unsigned `g0 < e[0x04]` | `(int32_t)g0 < (int32_t)e[0x04]` (0x3ce79/0x3ce7c `jge`) |
| B2 arm test | f.c:7744 | unsigned | `(int32_t)(g0 + 0xc8) > (int32_t)e[0x0c]` (0x3cf65/0x3cf68 `jle`) |

No guard was added to `blt_rows`; the fix corrects the caller's arithmetic only.

## 7. Green, same basis

`/tmp/rig/g311b` (fixed build, same drive, same pinned env, `NIGHT_BLITDBG=1`):

- 8 `blitdbg` arm-witness lines (the witness cap is 8 per arm), including the same B2 case that produced the
  red line: `arm=B2 mode=1 g0=1 e[0]=3280 e[4]=0 e[0xc]=200 e[0x1a]=320` ✓
- 0 `blt_rows h=` lines: no call leaves the shipped's `1..0xc8` band ✓
- 0 sanitizer/SEGV reports, `rc=0` ✓

The same 0-out-of-band census is re-run in the docs/312 sweep on the stage-2 warp basis (`--warp-episode 2`,
`ep1_st2w.txt`), which is the phase the playtester's fault happened in.

## 8. Rules this item produced (recorded in docs/65)

1. A row-pitch product that uses a column field is invisible to a screen comparison (the blit still draws
   *something*) and is visible the instant the row count is measured.  Measure the count, not the picture.
2. The witness must label the arm/site that produced the value.  "Something crossed a boundary" could not
   have distinguished the three `* 0x140` products in one arm.
3. Arm tests must be transcribed with their signedness (`jge`/`jle`), not with the C comparison that reads
   naturally.  A record with its sign bit set picks the other arm.
4. A fault log from a playtest is worth more than a description: the stack pointed at the helper, which made
   "the caller's arithmetic is wrong, not the helper's loop" the first hypothesis instead of the last.

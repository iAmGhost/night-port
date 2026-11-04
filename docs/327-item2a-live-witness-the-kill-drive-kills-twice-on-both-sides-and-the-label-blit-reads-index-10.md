# 327 — Item 2a has a live witness now: the same drive kills twice on both sides, and the EXP label blit is caught reading index 10

Goal task-21.  Item 2a is "the kill popup shows `1\n2\n3…` with a `0` prefix instead of `EXP`"
(docs/manual-playtest/03-2026-10-29.md).  It was fixed in docs/227 (`index 11 -> index 10` in `fn_03b718`'s tail
call) but the auditor's objection was that the claim rested on a **fixture**, because the canonical
`ep1_kill3` route "makes zero kills".  Here the same route makes **two kills on each side** and the label blit is
censused live.

## 1. Basis, and why the earlier "zero kills on both sides" reading was half wrong

| side | run | clock | kills (`DAT_0006339bc`) |
|---|---|---|---|
| shipped | `/tmp/rig/rb15/orig` (canonical frozen half, 150 s `ep1_kill3`, 412 samples) | real time in dosbox-automation | max **2** — 0->1 at `state_0076`, 1->2 at `state_0277` |
| port | `/tmp/rig/k327` (`--script scripts/rig/drives/ep1_kill3.txt --frames 12000 --rig-hz 8`, `NIGHT_SEED=13579`, **no** `NIGHT_RIG_STEP_CALLS` / `NIGHT_RIG_FPS`) | unpinned | max **2** — 0->1 at sample 731 (frame 4706), 1->2 at sample 1269 (frame 8237), 1 687 samples |

The popup list count `DAT_000639c8` is non-zero in **12** shipped samples and **29** port samples ✓ the same
structure on both sides.

The "zero" that docs/198 §2 and docs/211 §3 recorded is a **port-side, pinned-clock** fact: with
`NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60` this drive makes 0 kills and 0 popup posts, and docs/211 §3 already
says so.  It was never a shipped-side fact: the frozen shipped half reaches 2.  A kill census sentence must name
its clock (docs/190 §1, docs/191 §1 family); "the canonical route makes no kills" is only true of the pinned
port run.

```sh
make -C port app -B
d=/tmp/rig/k327; rm -rf $d; mkdir -p $d
env SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_GLYPHDBG=1 NIGHT_POPUPDBG=1 \
  stdbuf -o0 -e0 timeout -k 5 300 ./port/test/night --res-dir ./NIGHT \
  --script scripts/rig/drives/ep1_kill3.txt --frames 12000 --rig-hz 8 --rig-dump $d \
  > $d/out.log 2>$d/err.log
```

## 2. The kill popup, posted on the port side

```
popup post: a0+0=1 a0+0x17=21 a0+0x29=0 +0x32=10 ent+1=85  ent+9=178 latched=0
popup post: a0+0=1 a0+0x17=21 a0+0x29=0 +0x32=10 ent+1=350 ent+9=141 latched=0
```

Two posts, both carrying `+0x32 = 10` — the value `fn_015295`'s settle branch copies out of the victim at
0x0001541d (docs/211 §1) — on victims with `byte[+0x00] == 1`, i.e. counted by the same kill branch that
increments `DAT_000639bc` (shipped 0x00012a24-0x00012a33).

## 3. The EXP label census, live, on those same two popups

`fn_03b718`'s tail prints the array index it uses and the neighbour it must not use
(`port/src/f/f.c:15301-15310`, `NIGHT_GLYPHDBG=1`):

```
valuedisp_tail font=A count=11 arraysize=0x2c used[+0x28]=idx10 col=13 rows=6 glyph=0x1a7 | neighbour[+0x2c]=idx11 col=49 rows=49 glyph=0
valuedisp_tail_blit wrote=63 px bbox x 8..20 y 134..139 (13x6)
valuedisp_tail font=A count=11 arraysize=0x2c used[+0x28]=idx10 col=13 rows=6 glyph=0x1a7 | neighbour[+0x2c]=idx11 col=49 rows=49 glyph=0
valuedisp_tail_blit wrote=63 px bbox x 8..20 y 134..139 (13x6)
```

(the witness caps itself at 8 tail lines and 6 blit lines, so these are the first of the two popups; both popups
produce the same four lines.)

Reading that line against the shipped:

- `count=11 arraysize=0x2c` — `fn_03de85` sizes each parallel array at `count*4`, so `system.spr` (font A,
  `DAT_000639c0`) has **no** dword index 11 inside its allocation: `+0x2c` is one dword past the end.
- `used[+0x28]=idx10 col=13 rows=6 glyph=0x1a7` — index 10 is the record the shipped's own displacements name
  (0x0003b7f5 `add eax,[edx+0x28]`, 0x0003b801 `push [eax+0x28]`, 0x0003b80c `mov ecx,[eax+0x28]`), and
  `0x1a7` = 423 = the sum of the ten digits' payload lengths: the label sits exactly after the digits ✓
- `neighbour[+0x2c]=idx11 col=49 rows=49 glyph=0` — this is the pre-fix read: the adjacent heap dword `0x31`
  (49) used as *both* col and rows, glyph offset 0 = the first byte of the payload blob (the digit `0`).
  That is the reported symptom's mechanism: a `0` in front of a vertical pile.
- `wrote=63 px bbox x 8..20 y 134..139 (13x6)` — the label's actual footprint in the video plane: 13x6 cells,
  63 pixels written.  The pre-fix geometry would be 49x49 = 2 401 cells from one glyph stream.

So the census that used to exist only inside a unit test now exists in a run of the shipped drive, on the
records the game itself produced, with the kill count printed beside it.

## 4. What the shipped side does *not* give (basis limit, checked today)

The shipped popup node cannot be re-decoded from `/tmp/rig/rb15/orig`: `DAT_000639cc` there holds a **guest
linear pointer** (`0x383250`, `0x382fb8`) while each sample's `state_*.heap.bin` is a `0x5800`-byte window, so
the pointer is out of reach.  The node decode in docs/211 §2 (`flag=0 value=10 age=3 x=11 y=113`, font A control
block `advance 5 / rows 7 / col 4`) came from a different capture (`/tmp/rig/ents-orig/orig`, 352-byte pointer
cells) that is no longer on disk; it is cited as recorded, not re-verified.  The shipped-side claims quotable
today are the two DS-level ones in §1: kill counter reaches 2, popup list count non-zero in 12 samples.

Font-record content itself is asset-side and independent of either dump: `system.spr`'s record 10 is
`col 13 / rows 6 / glyph 0x1a7`, records 0-9 are the ten digits (docs/219, docs/221 §1) — which is why index 10
is the label and index 11 is nothing.

## 5. Fixture, as the secondary claim

`test_fn_03b718` (`port/test/test_f1.c:12465`) keeps the structural checks that a live run cannot pin:

- `A: terminator glyph from the control block at dest-0xf, 2 rows (rows/col from +0x28 = record 10, docs/227)`
- `B: terminator at dest-0x18 with start_row-0xd (rows 2, col from +0x28 = record 10, docs/227)`

plus the digit-run checks (five glyphs, 5 px advance, 7 rows for font A; `0xb` advance, 12 rows for font B) and
the age/list-splice behaviour.  They order the fix inside the function; §3 is the claim that the function runs
that way on a real kill.

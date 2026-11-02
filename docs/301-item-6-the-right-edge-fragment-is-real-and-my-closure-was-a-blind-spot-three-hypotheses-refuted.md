# 301 — item 6 (left-edge attack draws a limb at the RIGHT edge): the artifact is real, and my earlier closure was a blind spot

Date: 2026-11-02.  Playtest report `docs/manual-playtest/04-2026-11-02.md` item 5:
"when the player moves to the leftmost side, a partial sprite of the player appears at the right side of the
screen" (`scratch/graphic-glitch.png`).  This is goal item 6.

## 1. The closure in docs/298/docs/299 is void

docs/298 closed item 6 as "not producible by the port" on an event census (zero attack-state entries with
the player in the left half across four dumps), and docs/299 added `scripts/rig/limb_side.py` plus the
phase-matched drive `ep1_leftedge4.txt`.  Every one of those measurements asked a question *relative to the
player's own column* — how far ink reaches left/right of the player, which side the enemy approaches from,
whether the draw arm's anchor arithmetic matches.  **None looked at the screen's right edge**, which is
where the reported artifact actually is.  A census cannot find what it does not sample.  The item is
reopened.

## 2. The artifact, seen in the rig's own capture (no human input)

`/tmp/rig/le8` = `scripts/rig/route.sh scripts/rig/drives/ep1_leftedge4.txt 220` with
`NIGHT_RIG_SCREEN_EVERY=2` (5557 port witness frames).  Rendering the port's witness frame at t=104100 ms
(drive holds `left` from 85 s, an attack make lands at 104000 ms):

- the player stands at the far left, clipped by the screen edge;
- **at the extreme bottom-right, on empty floor, sits a small isolated object** — a fragment of a figure,
  not part of the room art (rendered PNG `/tmp/rig/le8_4781.png`).

A 2×2 comparison sheet `/tmp/rig/item6_sheet.png` puts side by side:

| tile | frame | what the bottom-right corner shows |
|---|---|---|
| port t=104100 | left held, attack | fragment present |
| port t=120033 | left held | fragment present |
| port t=71950 | mid-room, player at X≈+13 | corner clean |
| shipped t=103863 | same drive, same moment | corner clean |

So: on the port, the right-edge fragment appears while the player is pinned at the left bound and is absent
when the player stands mid-room; the shipped frame at the same drive time has no such fragment.  The phase
match is approximate (the shipped's player is in a different spot at that instant), so this is a confirmed
*port-side* artifact with a *suggestive* shipped-side contrast — not yet a gated two-sided divergence.

## 3. Three hypotheses I tested and refuted on the way

**a) "the port holds the draw origin fixed while the shipped scrolls it" — FALSE.**
`fn_012efa` subtracts `camX = g_dat_639dc` (DS 0x639dc) from world X.  Census on `/tmp/rig/do4`, both
sides, same drive:

| | distinct values | min→max | samples with a non-zero value |
|---|---|---|---|
| shipped (`state_*.ds.bin`, flat DS window at DS−0x50000) | 18 | 0→298 | 164/527 |
| port (`CellStream.val`) | 72 | 0→298 | 439/1278 |

Both sides scroll the origin over the same 0..298 range.  The first time I ran this census I hand-rolled the
port-side offsets and got "port = 852768 constant, distinct=1", which looked exactly like the hypothesis
being confirmed — it was a parser artifact (docs/289 §4: an instrument that reads a constant is broken, not
a finding).  Re-measured with the reader's own accessor (`CellStream.val`), the claim died.

**b) "a non-black-ink census at the screen edges will show it" — USELESS.**
`scripts/rig/edge_ink.py` on `/tmp/rig/do4` (980 witness frames at 320×200 RGBA): right-edge per-column mean
ink = **90.0 of 90 band rows** in the left-bound phase, in the mid phase, and in every other phase — the
room is bright in every edge column, so "is there ink here" saturates everywhere.  Reported as a broken
instrument, not as a pass.

**c) "the mirrored/reverse blit underflows its own row and lands on the previous row's right end" — NOT A
TRANSCRIPTION ERROR.**  The port's reverse branch (`f.c:7789-7812`) walks the destination leftwards from
`dest_off + col - 1`.  `dest_off < 0` makes `col += dest_off`, and the arithmetic bound on the tail is
exactly `-1`.  The shipped's own reverse tail says the same thing:

```
0x3be66  mov eax,[ebp-0x14] ; sub eax,[ebp-0x24]     ; skip = left - col_acc
0x3be79  push (rl - skip)                             ; length
0x3be80  mov eax,[rbp+0x14] ; add eax,skip            ; src
0x3be87  mov eax,[ebp-0x10] ; sub eax,[ebp-0x14]     ; dst = db - left
0x3be8e  call 0x40475                                  ; reverse copy
```

and the forward branches at `0x3bc4e-0x3bd69` match the port's three write forms
(`dst = db + left` at `0x3bd3f`, `dst = db + col_acc` at `0x3bce9`/`0x3bd13`).  `fn_03bb42`'s clipping is
byte-faithful in both modes ✓ so the artifact is not caused by a mis-clipped blit primitive.

## 4. One real static divergence found while checking (3 sites, unfixed)

All three sprite primitives bail on `dest_off > 0x140`:

| primitive | shipped guard | port guard |
|---|---|---|
| `fn_03bb42` | `0x3bb6a cmp [ebp-0xc],0x140 / jg bail` | `f.c:7736 if (dest_off > 0x13f) return;` |
| `fn_03bebb` | `0x3bee3 cmp [ebp-0xc],0x140 / jg bail` | same `> 0x13f` form |
| `fn_03c231` | `0x3c259 cmp [ebp-0xc],0x140 / jg bail` | same `> 0x13f` form |

The port bails one column earlier than the shipped: a sprite whose destination column is exactly 320 is
drawn by the shipped (writing at the next row's column 0) and rejected by the port.  It is the right shape
to fix (transcribe `0x140`) but it points the *wrong way* to explain a right-edge fragment on the port —
the port is the side that draws *less* there — so it is recorded as a faithful-transcription item, not as
item 6's root cause.

## 5. Where the investigation goes next

The primitive is faithful and the origin scrolls on both sides, so the remaining candidates are the
*inputs* the port hands the primitive: the per-sprite parallel arrays (`W`, `H`, `X`, `Y` from
`e+0x12b/0x12f/0x137/0x13b`, `f.c:15105-15111`) and the part/limb records' own offsets, which are what
determine whether a limb's destination column ends up near 320.  Next instrument: an edge witness inside
`fn_03bb42`/`fn_03c231` that logs, for the drive window where the player is pinned at the left bound, every
blit whose destination lands in the last columns of a row — with its `dest_off`, `col`, `start_row`,
`mode`, and the player's X — so the write can be attributed to a call site rather than inferred.

## 6. Standing rules this item produced

- Measure where the report says the bug is.  "Ink left/right of the player" and "ink at the screen's right
  edge" are different questions; the first was asked four times and the second never.
- An edge census must be discriminating on the route it runs on: check that a control phase reads *lower*
  than the suspect phase before quoting any number from it.
- Never hand-roll a dump parser to test a hypothesis that a parser artifact could also explain.

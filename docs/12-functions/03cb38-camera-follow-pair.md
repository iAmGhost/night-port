# 0x03cb38 / 0x03cbd9 / 0x03d4e4 — the camera follow pair and the policy that gates it

Ported 2026-07-13 (L11, L11, L12; 160 + 145 + 289 B) with one test of 18 checks. These three close the
horizontal-scroll chain that ends in the record decay of `0x03d75c`:

```
fn_03d4e4(room)                      ; camera policy: may the camera move this frame?
  -> fn_03cb38(DAT_00063354)         ; scroll right (slow 1 px, fast 3 px + snap)
  -> fn_03cbd9(DAT_00063354)         ; scroll left  (mirror)
       -> fn_03d75c(cam - prev)      ; distance-based float decay (EAX = travel delta)
  -> fn_03cc6b(DAT_00063354)         ; vertical follow / room-change state machine
```

## Shared state

| global | meaning here |
|---|---|
| `DAT_000639dc` | camera **x** (viewport left edge) |
| `DAT_000639e0` | camera **y** |
| `DAT_000639e4` | camera-locked latch, set/cleared by `fn_03cc6b` — freezes the horizontal camera |
| `DAT_00063354` | current **room record** — the "subject" the camera follows |
| `DAT_00053c7c` | **player entity**: `+0x01` x (dword), `+0x09` y (dword), `+0x17` state (dword) |
| `DAT_00053c68` | mode dword; the second bound pair only permits scrolling when it equals 1 |

Room-record fields used by this trio (dwords unless noted):

| offset | meaning |
|---|---|
| `+0x00` | room left x (the left follow target) |
| `+0x08` | room right edge (`cam + 0x140` is compared against it) |
| `+0x2a` / `+0x2e` | left / right scroll **stop** (0 = no stop) — a hard veto |
| `+0x32` / `+0x36` | second bound pair — crossing it downgrades the veto to "allowed iff `DAT_53c68 == 1`" |
| `+0x3a` / `+0x3b` | bytes: disable left / disable right |
| `+0x40` | room serial (used by `fn_03d75c`'s rate table) |

## fn_03cb38 — scroll right (EAX = subject)

```
prev = cam
if (DAT_639e4 == 0) {
    if ((int)player[+0x17] == 3) {                       ; fast follow
        if (subj[+8] - 0x142 > cam) cam += 3;
        else                          cam = subj[+8] - 0x140;    ; snap to the screen's right edge
    } else {
        if (subj[+8] - 0x140 > cam) cam += 1;            ; one pixel per frame
    }
}
if (cam != prev) fn_03d75c(cam - prev);
```

The snap branch can move the camera **backwards** (the test drives 860 -> 680), which is exactly why
`fn_03d75c` treats its delta as signed. When the camera does not move, `fn_03d75c` is not called at all
— pinned by leaving a seeded record's float untouched.

## fn_03cbd9 — scroll left (mirror)

```
if (DAT_639e4 == 0) {
    if ((int)player[+0x17] == 3) {
        if (subj[+0] + 2 < cam) cam -= 3;
        else                    cam = subj[+0];          ; NOTE: no -0x140 width offset
    } else {
        if (cam > subj[+0]) cam -= 1;
    }
}
if (cam != prev) fn_03d75c(cam - prev);
```

The asymmetry is genuine machine code: the right half snaps to `right - 0x140`, the left half snaps to
`left` itself, and the comparisons use `JG/JLE` vs `JGE/JLE` in opposite directions.

## fn_03d4e4 — the policy (EAX = room record)

```
if (room[+0x3b] == 0 && player[+1] - cam >  0xbe) {          ; player near the right edge
    if (room[+0x2e] == 0 || cam + 0x140 <= room[+0x2e]) {
        if (room[+0x36] == 0 || cam + 0x140 <= room[+0x36]) fn_03cb38(DAT_63354);
        else if (DAT_53c68 == 1)                            fn_03cb38(DAT_63354);
    }
}
if (room[+0x3a] == 0 && player[+1] - cam <  0x82) {          ; player near the left edge
    if (room[+0x2a] == 0 || cam >= room[+0x2a]) {
        if (room[+0x32] == 0 || cam >= room[+0x32]) fn_03cbd9(DAT_63354);
        else if (DAT_53c68 == 1)                      fn_03cbd9(DAT_63354);
    }
}
if (DAT_639e0 + 0xc2 > player[+9]) fn_03cc6b(DAT_63354);     ; vertical follow / room change
```

The two x thresholds (`0xbe` = 190, `0x82` = 130 of the 0x140-wide screen) leave a dead band in which
the camera does not move at all — a test sits in that band to prove nothing happens. The bounds are
checked in sequence: a non-zero `+0x2e`/`+0x2a` vetoes the move outright, while `+0x36`/`+0x32` only make
it conditional on `DAT_53c68 == 1`.

## Tooling note: sign-extend rel32 when hunting call sites

An earlier scan for `E8 rel32` targets reported *"no direct call sites"* for `0x03cb38`/`0x03cbd9`, which
tempted a wrong "called only through a pointer table" conclusion. The scan converted the 4-byte
displacement **unsigned**, so negative displacements never matched. The sites are plain direct calls:
`0x3d571 -> 0x3cb38` and `0x3d5d8 -> 0x3cbd9`, both inside `fn_03d4e4`. Correct pattern:

```python
rel = int.from_bytes(data[i+1:i+5], 'little', signed=True)   # signed!
if va + 5 + rel == target: ...
```

## Test map (18 checks, `test_fn_03cb38_pair`)

* right: slow step, edge veto (no move **and** no decay call), fast +3, snap backwards (delta −180 ->
  float +180), `DAT_639e4` freeze;
* left: slow step, fast −3, snap exactly to `room[+0]` (no width offset);
* policy: right gate `> 0xbe`, `+0x2e` veto, `+0x36` conditional on `DAT_53c68 == 1` (both sides),
  `+0x3b` disable, left gate `< 0x82`, vertical trigger (`fn_03cc6b`'s `g_639e0++` branch observed), and
  the dead band where nothing moves.

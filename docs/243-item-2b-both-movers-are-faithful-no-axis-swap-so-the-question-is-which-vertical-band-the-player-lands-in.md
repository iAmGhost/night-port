# 243 — Item 2b: both movers are faithful (no axis swap), so the divergence is which vertical band the player lands in

Follow-up to docs/242 §4, all static, to close the possibilities that were still open.

## 1. The two movers, mapped from the shipped bytes

| routine | fields it writes (corrected `field_writers.py`) | delta array it is fed |
|---|---|---|
| `fn_01545e` (0x1545e-0x1585f) | `+0x01` (**X**): `add [edx+1], eax` at 0x156e5, 0x156f3, 0x15786, 0x1583f; `mov [edx+1], eax` at 0x1570b, 0x157c4 | `[record + 0x71][frame]` (fixture-pinned, docs/146 §8) |
| `fn_01586b` (0x1586b-0x15a1e) | `+0x09` (**Y**): `add [edx+9], eax` at 0x159df, 0x159ea; `mov [edx+9], eax` at 0x159fe, 0x15a14 | `[record + 0x75][frame]` (shipped call site 0x12e01-0x12e17: `edx = [e+0x11] << 2; edx += [rec+0x75]; d = [edx]` ✓) |

The port matches both directions: `fn_01545e` works on `e + 0x01` (`f.c:9269+`), `fn_01586b` on `a0 + 9`
(`f.c:9467+`), and the arrays are heap blocks allocated and filled from the shipped resource stream
(`[p+0x75] = heap_malloc(count << 2)` + `fn_04192e(...)` ✓ `f.c:8783-8786` ✓, `XARR()` at `f.c:26376` ✓).

**So there is no axis swap and no missing table** ✓ docs/242 §4's "which array feeds which mover" question is
answered: the mapping is right ✓ The earlier confusion came from the port's own comments calling `fn_01586b`
"the X axis" routine (`f.c:9104`) ✓ -- it is the Y mover ✓ corrected ✓ (its clamp test reads `e+1` as the *key*
and clamps `e+9`, which is what made the comment look plausible ✓)

## 2. What is left, stated precisely

Measured: the shipped's player holds `+0x09 = 139` for 878 of 972 samples; the port's player spends 365 samples
at 172, 253 at 139, and drifts through 158 / 143 / 164 / 151 / 159 with the per-frame delta alternating ±1
(docs/242 §3, `/tmp/rig/y1`, `/tmp/rig/h3`) ✓ Both values are legal room positions: `fn_01586b`'s bounds are
`mn = DAT_00063354[+0x12] + camY = 139` and `mx = DAT_00063354[+0x16] + camY = 195` ✓ so **nothing in the Y mover
pushes a body off 172** ✓ The vertical bands inside that span come from the nine rows of the clamp table
`g_tab_5c8bc` (filled by the `.act` opcode-3 entries ✓ `f.c:27137` ✓ `rows=9` in the witness) and from the delta
array values themselves ✓

So item 2b's remaining question is: **which clamp row (if any) captures the player, and is the delta array the
port reads the same one the shipped reads for the same action record**? Two things to print, both cheap:

1. in `fn_01586b`: for the player, the row index whose `a1_lo < e[+1] < a1_hi` fired, the row's four dwords, and
   the `d` before and after the row adjustment (`f.c:9474-9484` ✓ the shipped's 0x15938-0x159bb halves);
2. at the `fn_01586b` call sites: the action record pointer, `e[+0x11]` (the frame index), and
   `[record+0x75][index]` as loaded -- then compare that against the same bytes in the shipped resource file
   (both sides read the same file, so this is a same-source comparison, not a hypothesis ✓).

If (2) matches and (1) shows the port being captured by a different row than the shipped's geometry implies, the
fix is in the row-selection (`a1_lo/a1_hi` are keyed on X ✓ so a wrong X range in a row -- i.e. an opcode-3
decode difference ✓ `f.c:27137` -- is the defect). If (2) differs, the defect is in the array load
(`f.c:8783-8786` ✓ count/stride).

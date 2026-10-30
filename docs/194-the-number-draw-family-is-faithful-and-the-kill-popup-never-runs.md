# 194 — Item 2a: the number-draw family is modelled faithfully, the blit's slots were my misreading, and the kill-time popup never runs

(The clarification that matters: item 2a's garbled text is the **EXP text drawn when an enemy dies**, not the HUD
field cropped in docs/191 §2.  That changes which routine is on trial — `fn_0383a4` / `fn_038c23`, not `fn_0137c1`.)

## 1. docs/193's argument-slot anomaly is retracted: it was a cdecl mis-application

Every function in this family sets its frame pointer **after saving three registers**:

```
0x3bb4c push esi ; 0x3bb4d push edi ; 0x3bb4e push ebp ; 0x3bb4f mov ebp, esp
0x383ae push esi ; 0x383af push edi ; 0x383b0 push ebp ; 0x383b1 mov ebp, esp
0x38c2d push esi ; 0x38c2e push edi ; 0x38c2f push ebp ; 0x38c30 mov ebp, esp
```

so relative to `ebp` the saved registers occupy `+4`, `+8`, `+0xc`, the return address sits at `+0x10 - 4 = +0xc`…
explicitly: at entry `[esp]` is the return address and `[esp+4]` the first stack argument; three pushes put `ebp`
twelve bytes lower, hence **argument 1 is at `[ebp+0x10]`, argument 2 at `[ebp+0x14]`, argument 3 at `[ebp+0x18]`**.

That is exactly what `fn_03bb42` reads — `rows` at `[ebp+0x10]`, the RLE pointer at `[ebp+0x14]` (advanced per run at
`0x3bbc7/0x3bbef/0x3bbf7`), the mode byte at `[ebp+0x18]` (`0x3bc25`) — and exactly what its callers push:
`fn_0137c1` pushes `0, glyph, 7` → mode=0, rle=glyph, rows=7 ✓; `fn_0383a4` pushes `glyph, 7` ✓.  The port's binding
`fn_03bb42(base, dest_off, start_row, col, row_count, rle, mode)` is therefore **right**, and docs/193 §2's
"call sites do not fill the slots the body reads" was my own slot-count error, recorded here so it is not repeated.
One residual: with only two stack words pushed by `fn_0383a4`, the shipped `mode` byte at `[ebp+0x18]` reads caller
stack, where the port passes `0` — plausible but unverified.

## 2. The family's glyph indexing, measured, and the port matches it

| routine | glyph displacement | digit `0` lands on | col × rows | per-slot x advance | port |
|---|---|---|---|---|---|
| `fn_0137c1` (HUD field) | `[char_table + d*4 - 0xbc]` (0x13891) | index **1** — the same entry as the blank pad | 5 × 7 | 6 (`imul …, 6`) | `f.c:13998` ✓ |
| `fn_0383a4` (integer) | `[char_table + d*4 - 0xc0]` (0x38419) | index **0** | 7 × 7 | 8 (`shl edx, 3`) | `f.c:14030` ✓ |
| `fn_038c23` (integer) | `[char_table + d*4 - 0xc0]` (0x38cad) | index **0** | 6 × 7 | 7 (`imul edx, edx, 7`) | `f.c:14527` ✓ |

So the three number-draw routines are faithful in the port in arithmetic, geometry and glyph selection.  The garble
cannot be an index or an advance.

## 3. The measurement that reframes item 2a

The glyph witness (`GLYPH_TRACE`, `f.c:3166`, enabled by `NIGHT_GLYPHDBG=1`, prints `tag=expnum … stream=<hex>`) fired
**zero times** on the canonical 150 s route (`grep -c menucell` on the run log = 0), and the same run reports
`exp award path: adds=0 added=0 commits=0 committed=0` with `fn_015295`'s attempts rejected 1 108 times at
`victim[+0x30] != 1` (docs/190 §1).  In other words, on this route **the port never reaches the code that draws a
kill's EXP number at all** — the same unfinished kill path that starves the award.

That makes item 2a and item 2b one defect with two symptoms, and it changes what "garbled" must be tested against:
the port's popup is not being drawn with wrong glyphs, the *kill completion path* that both the award and the popup
hang off is not completing (`+0x30` never reaching 1 in 1 108 of 1 396 attempts, `fn_035c5c` — the pass that sets
`+0x30 = 1` — reported `calls=0` in the same run).  The next concrete step is therefore `fn_035c5c`'s absence itself:
who calls it on the shipped side (its only known caller chain is `fn_01441e → fn_04185e / fn_03ee0a / fn_00035c5c` at
`0x150c8-0x150fa`, and the port models that trio at `f.c:25276-25280`) and why the port's copy runs zero times.

# 121 — A rig defect that hid ~80 declarations, what it exposed at DS 0x51b60, and the blind-gate re-run against a pre-fix build

Date: 2026-10-29. Rig files touched: `scripts/audit/port_src.py`, regenerated `scripts/audit/gen/*`,
`port/src/f/rig_cells.h` / `rig_table.h`.

## 1. Rig defect: the port-source parser could not see anything after a function body

`port_src._statements()` segments top-level statements to feed the declaration parser. Two bugs:

1. **A function definition ends with `}`, not `;`.** The scanner only closed a segment at a depth-0 `;`, so
   after every function body the segment stayed open and swallowed everything that followed: the statement
   containing `g_tab_51b60` measured **96,914 characters**, and `DECL.match` naturally failed on it.
2. **String literals were scanned for braces.** A `{` inside a log format string unbalanced the depth tracker,
   which is how the first bug went unnoticed (a stray `{` made a *whole range* of code look like one body).

Fixes: skip `"…"` / `'…'` bodies (with escapes) while scanning, and close a segment at a depth-0 `}` **only when
the text before the opening brace, comments removed, ends in `)`** — i.e. a definition signature, not an array
initializer (`static const uint32_t t[0x100] = { … }` closes at a depth-0 `}` too, and must not be split).

Measured effect (this is a rig-coverage improvement, not a game change):

| measure | before | after |
|---|---|---|
| port declarations parsed | 574 | **655** |
| …of which carry a DS address | 491 | **538** |
| rig state-dump cells | 277 | **283** |
| static audit (re-run) | — | c1_unmodelled 343, c2_aliasing 149, c3_image_init 29, c4_representation 3 |

## 2. What the newly visible declaration says: DS 0x51b60 is bigger than the reaction table

The reaction table (`g_tab_51b60`, docs/111) is now parsed with its initializer, and the parse makes something
plain that was invisible before: entries past index ~0x40 are **not reaction states at all** —
`0x9e9d9c00, 0xa09f, 0xffffffc9, …` and a run of IEEE-754 bit patterns
(`0x3f800000 = 1.0f, 0x3fa00000, … 0x40800000`) followed by an ascending ramp (`0xd4, 0xdc, 0xe6 … 0x4ff`).
DS `0x51b60` therefore aliases other data objects, and the port models that whole 0x400-byte span with one
array. That is the same one-address-many-objects family as docs/112 (`0x53d0c`) and docs/113 (`0x53d08`); the
audit files the overlaps it can see (149 c2 aliasing rows), and the open question is now explicit: **which
neighbouring DS objects inside 0x51b60..0x51bff are modelled by their own port symbol, and are their bytes
identical to the image at that offset?**

Oracle status for that cell: `SIZE? … no shipped size for this symbol; compared 256B` — the shipped-side
sampler does not yet read the newly mapped cells, so those cells compare with the port's width only. Extending
the emulator sampler's cell list to the regenerated table is the next rig step.

## 3. Blind-gate re-run against a pre-fix build (machine-only red-before)

Built the tree at `d22e4a7` in a worktree (`/tmp/prefix`) — the commit *before* the EXP alias
(`668aaa0`), reaction-table (`7f31de1`) and sound-slot (`ba30fb6`) fixes — and ran the **same drive**
(`scripts/rig/drives/ep1_kill3.txt`) against the same shipped reference dump, comparing both port builds with
the state oracle.

- DS-cell verdicts for the probe set are the same in both builds (the fixes live in heap records and in a table
  the oracle could not read until §1) — a candid statement that the *cell-level* oracle does not by itself
  discriminate those three fixes.
- **Record-level verdicts moved on 23 rows**, including:
  - `0x53dd4, 0x53dd8, 0x53ddc, 0x53de0, 0x53de4`: `REC-DIFF → REC-SAME` (five blocks now byte-equal to the
    shipped records),
  - `0x53c80` (`g_latched_ent`): `PTR → REC-SAME`,
  - `0x53c78`, `0x639d4`, `0x63358`: `REC-NULL/REC-DIFF → REC-SAME`.
- The pre-fix binary cannot produce entity-slot dumps (`--rig-ents` postdates it), so the entity-level red side
  is not reproducible on that build; the record-level comparison above is the honest substitute.
- New open item filed from this run: `0x53c70` / `0x53c74` went `REC-DIFF → REC-NULL` — the current build
  leaves those entity-chain pointer cells null where the shipped side holds live records. That is the
  docs/94 heap-offset family and is the next thing to triage.

## 4. Fresh route with the regenerated table (`/tmp/rig/g1`, ep1_kill3)

State oracle: **269 cells with activity profiles, tally `SAME` = 269** — no DS-cell divergence on the fixed
build for this drive. Screen: 6 runs below the 0.88 score gate and the pacing flag is still up
(picture changes per second orig 5.09 vs port 21.79, ratio 0.23), channel order ok (0.756/0.756, chroma 39.1).

## 5. Next rig steps, in order

1. Regenerate the **shipped-side** sampler's DS cell list from the same table so the 283 cells are compared
   with the shipped widths (removes the `SIZE?`/NO-DATA blind spot at 0x51b60 and friends).
2. Triage `0x53c70`/`0x53c74` REC-NULL (heap offset vs host pointer, docs/94 family).
3. Answer §2's aliasing question for 0x51b60 with the image bytes at file offset `0x51b60-0x7600`, and add the
   per-span overlap rows to the audit report.
4. Then the gate re-run can be stated at cell level too, not only at record level.

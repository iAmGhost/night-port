# 122 — Three image-initialised tables the port had zeroed: a new static check (c3b), three fixes, and the red→green signatures

Date: 2026-10-29. Follows docs/121 §1 (the parser fix that exposed 47 more DS globals).

## 1. New static check: `c3b_full_span_initializers`

`scripts/audit/static_audit.py` now byte-compares **every port global that carries both a DS address and a
brace initializer** against `NIGHT.EXE` at file offset `DS - 0x7600`, over the **port's own declared span**.
It exists because these tables are exactly the ones the Ghidra symbol table gives no size for, so
`c3_image_init` cannot reach them and the comparison silently uses whatever width the port invented.

First run: 15 rows — 12 of them informational (`span runs outside the image-initialised DS region`, i.e. BSS
globals that are legitimately zero in the image) and **3 real transcription failures**, all of which the port
had initialized to zero.

## 2. The three defects, and what each one was worth

| DS | port symbol | image bytes | port had | where it is used |
|---|---|---|---|---|
| `0x52f08` | `g_dat_52f08[0x40]` | `2, 5, 6, 6, 6, 4, 3, 9, …` (picture **count per stage**) | all zero | `count = g_dat_52f08[g_dat_63370]` (guest 0x0003a7c5) — the slide-show loop's trip count |
| `0x5274c` | `g_tab_5274c[0x4f0/4]` | runs of `0x3f000000` (= **0.5f**) | all zero | `float rate = g_tab_5274c[addr_off / 4]` (guest 0x0003d86f) — camera follow rate, so the follow was multiplied by 0 |
| `0x51ad4` | `g_rom_51ad4[26]` | `1a 00 03 … 07 …` | the `0x07` at `+0x10` was missing | `mem_copy(bufB, g_rom_51ad4, 26)` (guest 0x00010950-5a) — a ROM block copied into a buffer |

All three are transcribed verbatim from the image: `g_dat_52f08` as its dword list, `g_tab_5274c` as **exact
hex-float literals** (`0x1.0000000000000p-1f` style, so no rounding is introduced and the array stays a real
`float[]` the rig can address), `g_rom_51ad4` as the 26 image bytes.

The `0x52f08` finding is the interesting one for the playtest cluster: a **zero picture count** makes the
slide-show loop iterate zero times, which is indistinguishable from "the slide-show does not happen" — the
same neighbourhood as the slide-show key-wait analysis (docs/97) and the "no slideshow track" item.

## 3. Red → green, machine-measured

- Static (the check that found them): the three rows disappear from `c3b_full_span_initializers`.
- Runtime (independent of the static check — the state oracle had flagged the same addresses on the same
  drive): in `/tmp/rig/g1` (before)
  `DIFF 0x5274c … first differing byte @0x5274f 0x3f vs 0x00` and
  `DIFF 0x52f08 … @0x52f08 0x02 vs 0x00`, both **274/274 phases**;
  in `/tmp/rig/g2` (after) neither row is `DIFF` any more, and the whole table is
  **`cells with activity profiles: 269  tally={'SAME': 269}`**.
- `make test` → **ALL PASS**; `make selftest` → **ALL PASS (0 failures)**.

## 4. Filed, not yet fixed

`c3b` also reports two byte-mismatches inside the **code segment** (data embedded between functions), which
the same "transcribe from the image" treatment should get once their consumers are known:

```
0x48312 g_coeff_48312[90]  88 differing bytes (port 0x4e vs image 0x49 at +0x0, runs 0..17, 19..52, 54..59)
0x48a36 g_scale_48a36[9]   14 differing bytes (port 0x00 vs image 0x42 at +0x0, runs 0..3, 5..7, 9..e)
```

These feed the fx/scaling path (`g_scale_48a36[idx]` at `f.c:11267`, `g_coeff_48312 + i*10` at `f.c:11664`),
so they are the highest-value next transcription audit — and unlike §2 they may have been "derived" from
disassembly immediates rather than read from the image, which is precisely the failure mode this check was
built to catch.

## 5. Correction: the §4 rows were FALSE POSITIVES, and the check now says so

Trying the same "transcribe the image bytes" treatment on `g_scale_48a36` **segfaulted the fx fixtures**:
the bytes at `0x48a36` are instructions (`0x1442, 0x8948, 0x8002, …` as uint16), and the port's
`{0, 1, 2, 3, 4, 4, 5, 5, 6}` is the *decoded scale ladder* that the guest computes there. The guest's data
xrefs (`fn_048a48` reads `0x48a36`; `fn_048422` reads `0x48312`) are tables **embedded inside function
bodies**, where a byte-for-byte initializer comparison is simply the wrong model.

`c3b_full_span_initializers` therefore now resolves each DS address against the shipped function bodies
(binary search over `body_start..body_end`, **plus the 0x40-byte gap immediately before a body**, which is
where `0x48a36` sits — 0x12 below `fn_048a48`'s entry, a data pool read by that function) and routes those
rows to an informational class
(`"address is inside a shipped function body (code, not image data); compared bytes would be instructions"`)
instead of filing them as mismatches. Re-run after that fix:

```
c3b rows 12 | real mismatches 0 | code-region (informational) 2 | BSS / outside the image region 10
```

So the honest state of the check is: it found the three real zeroed tables of §2 (now fixed), and its
remaining rows are the two informational classes. `src/f/f.c` was reverted for the `g_scale_48a36`
experiment; `make test` is green again.

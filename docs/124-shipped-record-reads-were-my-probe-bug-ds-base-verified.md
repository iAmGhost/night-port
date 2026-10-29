# 124 — The "shipped side reads zero / heap-relative pointers" blocker was my own probe bug: DS base verified, pointer cells live, heap blocks live

Date: 2026-10-29. Re-examining the Blocked item from earlier turns ("original-side DS cells
`0x53c7c/0x53c80/0x53c84/0x53c88` read 0 across all 572 samples").

## 1. Correction

The blocker does **not** exist. The probe that produced it read the DS snapshot with `DS_LO = 0x51000`
while the rig's dump starts at `0x50000` (`replay_original.py:33`), so every address was shifted by one
page and the values printed (`0xac`, `0xa2`) were adjacent unrelated bytes.

Re-read with the correct base, over all 1300 shipped states of `/tmp/rig/g2`:

| cell | distinct values | max |
|---|---|---|
| `0x53c70` | 5 | `0x5756c8` |
| `0x53c74` | 13 | `0x575960` |
| `0x53c7c` | 3 | `0x5756c8` |
| `0x53c84` | 3 | `0x5756c8` |
| `0x53c88` | 4 | `0x383250` |

and the corresponding blocks in `state_NNNN.heap.bin` are non-zero record bytes (e.g. 67 non-zero bytes at
the `0x53c70` block, `00 5a 00 00 00 00 00 00 00 8b 00 ...`). So `read_at(v)` on the guest value returns real
record content; the guest stores **flat-model pointers**, not heap-relative offsets, in these cells.

## 2. DS physical base is verified byte-exact (new check worth keeping)

Image-initialized DS cells in the shipped dump equal `NIGHT.EXE` at file offset `DS - 0x7600`:

```
0x51b60 (reaction table)  MATCH
0x52f08 (picture count)   MATCH
0x5274c (camera rates)    MATCH
0x51ad4 (ROM bytes)       MATCH
0x53d0c / 0x639bc         no match -- runtime cells, expected
```

This is the strongest available proof that the DS physical calibration is right, and it is *absolute*
(matches the shipped image), unlike the digest-equality test which only proves self-consistency.

## 3. Record verdicts now localize to named fields (rig change, with an honest caveat)

`diff_state.py` previously reported a record diff as "first differing byte". It now prints the differing-byte
count plus candidate fields (widths taken from the shipped access width at each offset), e.g.

```
REC-DIFF orig=92B differ; rec+0x00=0x01 port=rec+0x00=0x00
        <== score@+0x52 0x20/0x0; act_container@+0x123 ...
REC-DIFF orig=46B differ; ... <== state@+0x17 0x2800/0xe800; next_ent@+0x13f ...
```

**Caveat that must be resolved before quoting field names as findings:** the values show the offsets are off
by one byte — `state@+0x17` reads `0x2800`, i.e. the live state byte is at `+0x18` (and the same shift is
visible for the other candidates). The candidate table therefore needs a one-pass calibration against values
whose behaviour is already known (`+0x18` state = `0x28` for the idle/enemy state both sides agree on) before
the field names carry evidential weight. The byte-level verdicts themselves are unaffected.

## 4. Consequence for the open gate item

Entity-level blind-gate items (EXP-per-kill, fallen reachability) are **not** blocked by any shipped-side read
limitation: the shipped records are readable today through `state_NNNN.heap.bin`. What is missing is field
offset calibration on both sides, and that is now the named next step.

# 171 — The walk-step table is byte-identical to the image; the `d == 0` deficit is not coming from it

docs/152 §4b left a candidate: the player mover `fn_01545e` returned early on 3167 of 3715 player calls
(`d == 0`, guest `0x1547b-7f`), i.e. the player "walks on roughly one frame in seven", against a shipped
signature of ~1 px per frame.  One of its two step sources is a DS table, so the first thing to check was the
table's bytes.

## 1. The table

`f.c:26912` builds the step index as

```c
int32_t step = (int32_t)g_tab_52ffc[(((int32_t)g_dat_63360 % 4) * 3) + PL8(0x25)];   /* (frame % 4) * 3 + slot byte */
```

`g_tab_52ffc` (DS `0x52ffc`, 65 dwords) read out of the **port's own memory** through `cells_reader.py` and
compared byte-for-byte against `NIGHT.EXE` at file offset `0x52ffc - 0x7600`:

```
cell host_size 256  compared 256  ->  byte mismatches: 0
image dwords 0..13: 1 2 2 1 1 2 1 2 1 1 1 1 0 0
port  dwords 0..13: 1 2 2 1 1 2 1 2 1 1 1 1 0 0
```

Two facts fall out.  The transcription is faithful (this table is *not* a docs/122 zero-table defect).  And the
image's step values are only ever **1 or 2**, non-zero only for indices 0..11 -- which is the shipped walk
signature in one line: a walking frame moves 1 or 2 pixels, and an index of 12 or more (slot byte 3 and up)
selects a zero step and produces exactly the `d == 0` early return.

## 2. Which index the port actually selects

A sampling witness at the mover entry (`NIGHT_WALKTRACE`, `ep1_kill3`, 12 000 frames, fixed step 665/60):

```
walkslot lines: 156
  slot25=0    sampled=155   d==0 -> 112
  slot25=255  sampled=1     d==0 -> 1
```

The player's slot byte `+0x25` is **0** in 155 of 156 samples, so the index is `(frame % 4) * 3` in
{0, 3, 6, 9} -- and all four of those image entries are 1, i.e. **the table cannot be the source of the zeros**.
Yet `d` is still 0 in 112 of 155 samples (72 %).

So the early returns come from the *other* step source, the per-frame delta arrays of the `.act` action record
(`record[+0x71][frame]` for Y via `fn_01545e`, `record[+0x75][frame]` for X via `fn_01586b`, docs/146 §8), whose
non-zero entries are sparse by design: most frames of an action carry no delta at all.

## 3. What this closes, and what it does not

- Closed as **not-a-defect**: `g_tab_52ffc` needs no transcription, and the port selects table entries the shipped
  game also selects.
- The `d == 0` rate is *expected behaviour of the action records*, so it is not by itself evidence of a mover
  defect.  The open question is whether the port's action records *are* the shipped records -- the same chain
  question docs/110 / docs/155 / docs/167 §15 keep returning to, now with a specific measurable consequence
  attached: how many non-zero delta entries a record has per frame.
- The comparison that would settle the mover question needs a shipped-side `fn_01545e` call census, and there is
  no instruction-level oracle (the emulator's `debugger` capability is `false`).  What *is* available is
  position-band comparison, and docs/152 already shows the two sides' px-per-step products close
  (shipped 1.40 px/step at 43.3 steps/s, port 1.64 px/step at 42.7 steps/s) -- consistent, not divergent.

Witness added: `walkslot` line in `fn_01545e` under `NIGHT_WALKTRACE`.

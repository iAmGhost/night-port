# 132 — State-oracle triage: the value oracle was never 269/269 green; 37 cells DIFF, and a new port-zero family

## 1. Self-correction first

Several docs quote "state oracle: 269/269 SAME". That number is the **activity-profile** oracle
(`diff_activity.py` — which cells change, in which phase). The **value** oracle (`diff_state.py`) has never
been fully green on a full-length route. On both `ep1_kill3` routes measured here:

| run | cells | verdict tally | distinct DIFF cells |
|---|---|---|---|
| g2 (300 s) | 344 | `SIZE? 31, PTR 52, SAME 53, REPR 22, DIFF 111, ENTROPY 14, REC-DIFF 32, REC-NULL 16, …` | 37 |
| g6 (200 s, capture rate fixed, docs/130) | 344 | `SIZE? 32, PTR 52, SAME 53, REPR 22, DIFF 110, ENTROPY 14, REC-DIFF 36, …` | 37 |

So the DIFF set is **stable and reproducible**, not a regression from the alignment refactor or the docs/122
table fixes — and it is the same 37 cells on both routes. Anyone quoting "green" for the state oracle has to
mean the profile oracle; the closeout contract's "state diffs green" is **not yet met**, and this doc is the
triage that says what stands in the way.

## 2. Is it mis-phasing? No (measured)

New probe `scripts/rig/drift_probe.py`: resample both value traces onto a 2 Hz grid and find, per cell, the
time shift (±60 s, 0.5 s steps) that maximizes the fraction of equal samples. If the two sessions were simply
displaced, a common nonzero shift would make the cells agree.

```
cells that become >90% equal under some shift: 4 of 37
their shifts: median +0.8 s, min +0.6 s, max +0.8 s
cells never >90% equal: 33
```

Four cells align at +0.8 s (that is real, small, and consistent — the boot offset), the other 33 do not align
at any shift. So these are value divergences or comparison-basis defects, not phase artifacts.

## 3. Composition of the 37 DIFF cells

* **10 rows: "no shipped size for this symbol"** — the oracle cannot know the guest width, so it compares the
  host declaration's size (e.g. `0x51c24 g=0 h=256`). Verdicts there are not meaningful until the Ghidra
  interval is imported for those symbols. This is a rig gap with a known fix (feed `ds_join`/ds_symbols widths).
* **3 rows: explicit width disagreement** (`0x537a7` 1 B guest vs 4 B host; `0x53818` 4 B guest vs 256 B host)
  — the oracle compares only the first guest-width bytes and says so. Also rig-side debt: these rows should be
  verdicted as `WIDTH?` rather than `DIFF`.
* **28 rows: the port holds 0 where the shipped side holds a value.** Named, with write-site counts in `f.c`:

```
DS 0x5333c g_ss_saved        writes=2     DS 0x53384 g_heap_seg_limit   writes=5
DS 0x53358 g_heap_highwater  writes=6     DS 0x53394 g_stack_bound      writes=1
DS 0x533b2 g_heap_cfg_b2     writes=2     DS 0x51c98 g_dat_51c98        writes=5
DS 0x530ac g_dat_530ac       writes=2     DS 0x53619 / 0x5363c / 0x53640 / 0x536dc  (1-2 writes each)
DS 0x533b9 0x533bb 0x53451 0x5377c 0x53780 0x53784 0x53788 0x537a7      (odd, sub-dword offsets)
```

Two observations that make this family worth a dedicated pass:

1. Half the names are **DOS-extender memory-map state** (heap highwater, heap segment limit, stack bound,
   saved stack pointer). The shipped process has real values there because the extender filled them at init;
   the port models the *variables* but nothing writes them during its own init. That is a legitimate
   equivalence question (docs/94's heap-offset hypothesis family touches exactly these cells), but it is also
   the kind of divergence with **no gameplay effect**, so it must be classified rather than "fixed".
2. The odd addresses (`0x533b2`, `0x533b9`, `0x533bb`, `0x53451`, `0x53619`, `0x537a7`) are byte/word fields
   at non-aligned offsets inside a DS struct. Those are the same field-offset calibration hazard recorded in
   docs/115 (record offsets must come from the dump's explicit `offset` field) and docs/125 (the entity layout
   is deliberately unaligned). A 1-byte guest field compared against the first byte of a 4-byte host global is
   only valid if the port's declared type matches the guest width — which the `WIDTH?` rows above prove is not
   always true.

## 4. What this changes

* The closeout item "state diffs green on the fixed build" now has an explicit work list instead of a slogan:
  (a) import guest symbol widths for the 10 size-unknown cells, (b) verdict width-mismatch rows separately,
  (c) triage the extender-memory-map family as *modelled-but-unwritten* (probably a documented non-divergence),
  (d) re-check the odd-offset byte fields against the struct layout.
* The blind gate's evidence base is unaffected: every detection reported in docs/105/111/113/125/131 came from
  the profile, screen, entity, schedule and audio oracles, and the audio music detection (docs/131) is now
  independently replicated (docs/131 §5).

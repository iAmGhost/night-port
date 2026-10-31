# 277 — Item 2b: the shipped and the port post and clear the ring the *same number of times* — the posting-starvation story is refuted, and so is my fill-percentage inference

Event census on the canonical dumps (`/tmp/rig/rb8`, `ep1_kill3.txt 150`, shipped via its flat DS window at
`offset = DS − 0x50000` ✓, port via its cell stream ✓), counting *transitions between consecutive samples* rather than
states ✓:

```
                  0 -> code transitions   code -> 0 transitions   samples
SHIPPED rb8              21                      11                 428
PORT    rb8              21                      11                1500
```

## 1. Two claims die here

* **"The port posts too few ring entries."** ✗ Refuted ✓: the observable post events are *identical* (21 vs 21 ✓), and
  the live in-process counter (122 `fn_034353` entries ✓ docs/276 §1 ✓) is the same order as the shipped's posts plus the
  writes that sampling at 10 Hz cannot separate ✓ The arming/posting path (`keymap.c:63`'s `held == 0` gate ✓, the typematic
  repeat I added ✓) is therefore **not** where item 2b is lost ✓ docs/272 §3's and docs/275 §2's framing is superseded ✓
* **"Ring fill 86 % vs 33 % proves starvation."** ✗ Withdrawn ✓: those are *state* censuses taken at different sampling
  cadences (shipped 428 samples ✓, port 1500 samples ✓), and a post may overwrite an already non-zero slot without
  appearing as a `0 -> code` transition ✓ Comparing fill percentages across cadences is the same error class as
  docs/264 §7 and docs/186's basis rule ✓

## 2. What still stands as the divergence (event-shaped, both sides)

* Chain target selection: shipped `DAT_00062d58 = {0x33 ×1453, 0x35 ×198, 0x34 ×108, 0 ×78, 0x36 ×9}` ✓ vs port
  `{0, 0x33}` ✓ (docs/264/266 ✓, re-confirmed in docs/272 §2 on rb8 ✓) — the multi-code technique entries still never
  complete in the port ✓
* Ring cursor advance share: shipped advances on 38 % of its sample pairs, port on 7 % of its own ✓ — but since the
  *event* counts match ✓, this too is cadence-shaped, and only the event counts should be quoted ✓
* `fn_03442e`'s combination branches never fire on either side (no `0x67/0x68/0x69` in either ring ✓) ✓, and
  `fn_0348e2` is rare in the port (13 ✓) ✓

## 3. Next step, unchanged and now sharpened

Per-entry rejection attribution inside `fn_03272f`'s chain walk on the dump-backed basis (docs/264 §5's plan): which
entry index fails at which stage, with the `act_ring_*` / `act_cursor_*` probes ✓ — the divergence is in the window the
resolver examines (docs/266: `code0 mismatch`, `slot consumed`, `len != k` rejections vs 53 matches, target `0x33`
only ✓), not in how the ring gets filled ✓ Gates for any `fn_03272f` change: matched targets gaining `0x34/0x35/0x36` ✓,
a settled `kind==1` enemy ✓, `+0x32 = 10` ✓, `player[+0x52]` rising ✓, suite ALL PASS ✓, soak 9/9 ✓. No code changed in
this step, so the suite result from docs/275 (ALL PASS ✓) still describes HEAD `17c68a0` + the witness counters ✓

# 257 — Item 2b: the shipped's player enters states 0x34 and 0x35; the port's never does (reproducible on two runs each)

Dump-side state-transition census on the player record (`kind == 0`, `+0x29 == 1000` ✓ transitions between
consecutive samples of `+0x17 & 0x7fffffff` ✓ no new instrumentation ✓ two runs per side ✓):

```
SHIPPED /tmp/rig/ents-orig/orig (407 s)
  out of 0x1: 0x1->0x2 28, 0x1->0x34 20 ✓, 0x1->0x35 7 ✓, 0x1->0x33 6, 0x1->0xb 4
  into  0x1: 0x33->0x1 26, 0x2->0x1 12, 0x34->0x1 11 ✓, 0x35->0x1 11 ✓, 0xc->0x1 3, 0x0->0x1 1
SHIPPED /tmp/rig/g37/orig (150 s)
  out of 0x1: 0x1->0x2 24, 0x1->0x34 16 ✓, 0x1->0x35 8 ✓, 0x1->0xb 3, 0x1->0x33 3, 0x1->0x15 2
  into  0x1: 0x33->0x1 21, 0x35->0x1 13 ✓, 0x34->0x1 9 ✓, 0x2->0x1 7, 0xc->0x1 3, 0x0->0x1 1

PORT /tmp/rig/rb3/port (150 s, after the docs/254 clamp fix)
  out of 0x1: 0x1->0x33 33, 0x1->0x2 24, 0x1->0xb 5            (0x1->0x34: 0 ✗, 0x1->0x35: 0 ✗)
  into  0x1: 0x33->0x1 39, 0x2->0x1 17, 0xc->0x1 5, 0x0->0x1 1 (0x34->0x1: 0 ✗, 0x35->0x1: 0 ✗)
PORT /tmp/rig/v5a/port (600 s)
  out of 0x1: 0x1->0x33 41, 0x1->0x2 29, 0x1->0xb 3            (0x1->0x34: 0 ✗, 0x1->0x35: 0 ✗)
  into  0x1: 0x33->0x1 54, 0x2->0x1 14, 0xc->0x1 5, 0x0->0x1 1 (0x34->0x1: 0 ✗, 0x35->0x1: 0 ✗)
```

Two independent runs per side ✓ identical shape ✓✓ — the divergence is **structural, not variance** ✓:

* the shipped leaves the walk state 0x1 into **0x34** (20 / 16 times ✓) and **0x35** (7 / 8 times ✓) and returns
  from both (11 / 9 and 11 / 13 ✓);
* **the port never enters 0x34 or 0x35 at all** ✓ in either run ✓ and instead runs a tight **0x1 <-> 0x33** cycle
  (33 out / 39 back at 150 s ✓ 41 / 54 at 600 s ✓) at 5-7x the shipped's 0x33 rate (6 / 26 ✓) ✓

Combined with docs/256 (the descent happens in state 0x1 ✓ the alternation is state 0x2 ✓ the record delta arrays are
inert ✓) and docs/255 (the enemies' line is 141 ✓ the shipped parks the player at 139 = in band ✓ the port holds
172 = 31 px out of band ✓), the chain now reads: the shipped's player cycles walk -> 0x34/0x35 -> walk ✓ which is
where its monotone descent into the bound happens ✓; the port cycles walk -> 0x33 -> walk ✓ and stays high ✓

## Where 0x34 / 0x35 come from in the shipped (candidate store sites)

`imm_scan` over the whole code range:

| immediate | functions containing it (1-2 hits unless noted) |
|---|---|
| `0x34` | fn_011827, fn_01194c, fn_012616, fn_016750, fn_027bec, **fn_02fe1e (2)**, **fn_03272f**, fn_034f31 |
| `0x35` | **fn_01194c**, **fn_01441e**, fn_03ace9 (4x `mov ebx, 0x35` at 0x3ad5d / 0x3ada2 / 0x3ae30 / 0x3ae71), fn_041ec3 |
| `0x33` | fn_0113e7, **fn_01194c (4)**, fn_01441e, **fn_03272f (2)**, fn_0433ce, fn_04a9da |

The port contains **no `PL32(0x17) = 0x33 / 0x34 / 0x35` literal** ✓ (its `fn_03272f` state stores are 0x83-0x85,
0x97-0x9c, 0xa3-0xa9 ✓) — so either the port produces those states through a computed value (a range/table mapping ✓
which is how the shipped's `fn_01194c` reaction family looks likely to work ✓ given it holds 4 writes of 0x33 and one
each of 0x34/0x35 ✓) or the transition genuinely never happens ✓ **This is the localization target for the remaining
half of item 2b** ✓ and it is checkable byte-by-byte: read `fn_01194c`'s 0x1194c-body region around its `0x33/0x34/0x35`
immediates ✓ compare against the port's `fn_01194c` model (`f.c:26223`+ ✓ 3269 B shipped ✓) ✓

## Item 2b status, precisely

* **Fixed and evidenced:** `fn_01586b`'s clamp guard (`cur < mn || cur > mx` -> the shipped's `cur > mx` ✓ docs/254
  ✓): player at the bound 268 -> 406 samples ✓ enemy hits 8 -> 11 ✓ debt floor 29 -> 25 ✓ state-oracle DIFF 35 -> 34
  ✓ suite ALL PASS ✓
* **Localized, not yet fixed:** the player never enters 0x34/0x35 ✓ (this doc ✓) — the states whose walk-cycle puts
  the player inside the enemies' 141 band ✓ Without them no route reaches debt 0 (best: 17 ✓ docs/255 §3 ✓) so no
  settle, no queue post, no `player[+0x52]` award, and the shipped's `type 31 -> +0x32 = 10` per-type EXP stays
  unmatched in the port ✓
* Faithful and exonerated so far: the award routine and its two queue posts ✓ the per-type table ✓ the damage model
  ✓ fn_013c4d's Y gate, X box arithmetic and facing polarity ✓ (docs/255 §2 ✓) both movers' clamp bodies ✓ (only the
  guard was wrong ✓) the walk step table ✓ fn_03272f's settle conversions ✓ (docs/254 §3 ✓)

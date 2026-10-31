# 261 — Self-correction: the 0x2e bytes in gun.act are not per-row reaction codes, and the port's live action rows are zero there

## 1. The port's live action records, dumped (`NIGHT_CHAINTRACE=1`, `/tmp/rig/ch1`, canonical 150 s)

Six distinct action records the player/entity chain actually resolved (`RECDUMP` ✓ `f.c:26567-26598` ✓), showing the
16 bytes at `+0x38..+0x47` — the window `fn_01441e` reads as the reaction code (`byte[rec_e + slot + 0x38]` ✓):

```
rec=0034b8cc ptbl=0034b744 st=1   +0x38..+0x47: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
rec=0007213c ptbl=00071d88 st=1   all zero
rec=00072440 ptbl=00071d88 st=2   all zero
rec=0034b970 ptbl=0034b744 st=2   all zero
rec=000778bc ptbl=00071d88 st=51  +0x38..+0x47: 02 00 00 00 00 00 3b 00 00 00 00 00 25 00 00 00
rec=0034c318 ptbl=0034b744 st=12  all zero
HIT atype census for the run: {1: 7, 2: 13, 11 (0x0b): 10}
```

So most of the port's action rows carry **zero** in the reaction-code window ✓ (an `atype` of 0 maps to
`g_tab_51b60[0] = 0` ✓ — i.e. the victim's state is set to **0** ✓ which matches the port's player spending 241
samples in state 0 ✓ docs/254 §4 ✓ — a consistency the rig did not have before ✓). The one non-zero row supplies
`+0x38 = 0x02` ✓ plus fields `+0x40 = 0x3b` and `+0x46 = 0x25` ✓ that are *not* read as reaction codes ✓

## 2. Retraction: "0x2e is present in the port's .act data" was an over-reading

docs/260 addendum 2 counted `0x2e` 44/45/46 times in `gun.act`/`gun1.act`/`gun2.act` ✓ That count is real but the
inference is not:

* only **2** of `gun.act`'s 44 occurrences lie below `0xd000` (`0x1d02`, `0x5b84` ✓) — the other **42 are one
  consecutive run at `0xdbd9, 0xdbdb, 0xdbdd, 0xdbdf, 0xdbe1, 0xdb e3 …`** ✓ every second byte ✓ i.e. a *data
  array* (16-bit values whose low byte is 0x2e ✓), not per-row fields ✓;
* scanning every candidate row stride 4..512 for a stride where those isolated `0x2e` offsets land at
  `row_base + 0x38`: **no stride qualifies** ✓ → `0x2e` is **not** evidenced as a per-row reaction-code value in the
  shipped's `gun.act` ✓

So docs/260 addendum 2's conclusion ("the data is there, the port just reads the wrong row+slot ✓") is **withdrawn**
✓ and docs/65 carries the correction ✓

## 3. The honest open question, and the measurement that answers it

Two possibilities remain, and they are distinguishable:

1. the shipped's **in-memory** action records are *not* the `.act` file bytes ✓ (`act_block()` resolves a resource
   block, and both sides post-process `.act` rows into the 0x88-slot record layout the port models ✓ — so the
   reaction code the shipped reads at `rec_e + slot + 0x38` may be produced by that expansion, not stored in the
   file ✓), or
2. the port's expansion writes a different byte at that offset than the shipped's expansion does ✓

Measurement that separates them without guessing: read the **shipped's live action record** — the rig already reads
shipped record bytes out of its DS-mapped heap (`docs/124`'s shipped-side record reads ✓, and the `wait ds 0xADDR`
mechanism reads arbitrary DS dwords and records `{t_ms, ds, observed, hit}` ✓) — sampling the dword at
`rec + 0x38` for the record the shipped's player chain resolves to, and comparing it against the port's
`+0x38 = 0x00/0x02` ✓ If the shipped's value is `0x2e` there ✓ possibility (2) is confirmed and the fix is in the
port's `.act` expansion (a transcription of the expansion, gated by `HIT ... atype=46` ✓ and the 0x33/0x34/0x35
state transitions appearing ✓) ✓ If it is also 0 ✓ then the shipped's 0x33/0x34/0x35 states come from a *different*
writer and the search reopens at the state-store census ✓

Item 2b status is unchanged otherwise: clamp-guard fix landed and evidenced ✓ (docs/254 ✓); the award path, table,
damage model, hit gates, `g_tab_51b60` and the reaction block all proven faithful ✓ (docs/255-260 ✓).

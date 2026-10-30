# 178 — The residual state rows, sorted by what their series actually looks like

35 residual same-width gameplay divergences were reported on the canonical route (`/tmp/rig/g27`, `ep1_kill3`,
150 s, `RIG_HZ=10`).  Rows had been enumerated forever as "value divergences", which hides the fact that they are
four different problems.  `scripts/rig/residual_shape.py` reads each cell's whole series on both sides -- 853 shipped
samples, 1500 port samples -- and labels its shape.  It is deliberately **unaligned**: it compares value sets and
monotonicity, not per-window equality, so it produces routing information, never a verdict (alignment remains the
job of the transition anchor, docs/158).

## 1. Census of the 35 rows

| Shape | rows | what it means |
| --- | --- | --- |
| **SAME VALUE SET, different order/dwell** | 14 | both sides visit the same values; only the sequence and dwell times differ.  `0x51c98 {1,2}`, `0x53c48`, `0x53c6c {0,1}`, `0x62a8c {0..9}`, `0x62a90 {0..8}`, `0x62a94`, `0x6335c {0..4}`, `0x639d8`, `0x639e8`, `0x63ea5`, `0x63ed0`, `0x63ed3`, `0x63ed5`, `0x63ed8` |
| **PARTIAL overlap** | 6 | shared alphabet, different reach: `0x53100` (shipped `[3..3]`, port `[2..3]`), `0x53c68` (`[0..7]` vs `[0..9]`), `0x62d58` (`[0..53]` vs `[0..51]`), `0x63364` (`[0..2]` vs `[0..19]`), `0x6339c` (`[0..41]` vs `[0..202]`), `0x63e14` (`[0..88]` both, 5 vs 8 distinct) |
| **DISJOINT value sets** | 3 | `0x53c88` (`[0..3682896]` vs `[0..3831936]`), `0x63360` (`[0..6986]` vs `[0..5748]`, docs/177), `0x639dc` (`[0..182]` vs `[0..298]`) |
| **Constant absolute address** | 5 | `0x53c64` 1691872, `0x63ffc` 1691956, `0x633b0` 2207800, `0x633b4` 2273336, `0x63e18` (uptime stamp) |
| **Port all zero, shipped holds a value** | 6 | `0x53451` (=4, the `fn_04933a` coverage gap of docs/173 §6), `0x62d5c`, `0x63e20`, `0x64000` (=189143), `0x64090`, `0x64094` (=1691857) |
| **Shipped all zero, port holds a value** | 1 | `0x63e5c` (port 1) |

## 2. Five rows reclassified with the measured evidence

- **`flat_pointer_slot`** (new family): `0x53c64`, `0x63ffc`, `0x633b0`, `0x633b4`.  Each holds a *single constant*
  for the whole route in the allocated-arena range (1.69M-2.27M), on both sides.  Two processes cannot agree on
  absolute addresses -- that is the same situation docs/134 §3 already accepted for the heap cursor cells, and the
  comparable statement is relative to each side's base, which the oracle does not do yet.
- **`clock_stamp`** extended with `0x64000` (`g_clock_ticks`: shipped constant 189143 across 853 samples, port 0 --
  a frozen tick stamp rather than a counter) and `0x63e20` (shipped absolute values up to 5567560, port 0, sitting
  just outside the `0x63e24..0x63e40` block docs/101 entropy-masked).

Re-running the oracle on the same dumps:

```
non-gameplay families: host_address_space 2, device_plumbing 27, arena_state 3, dos_extender 10,
                       flat_pointer_slot 4, clock_stamp 3
residual same-width gameplay divergences: 28       (was 35)
```

## 3. What the remaining 28 actually are

```
14  same value set, different order/dwell   -> a timing/population question, not a state-model question
 6  partial overlap                         -> reachable-set width (0x6339c [0..41] vs [0..202], 0x63364 [0..2] vs [0..19])
 3  disjoint sets                           -> 0x53c88 (heap-offset semantics), 0x63360 (docs/177 route end), 0x639dc
 4  port holds zero, shipped holds a value  -> 0x53451 (needs fn_04933a to run), 0x62d5c, 0x64090, 0x64094
 1  shipped holds zero, port holds a value  -> 0x63e5c
```

That reframes the closeout item.  Fourteen of the remaining rows are **not** candidate model defects at all: the
two sides produce the same values in different order and dwell, which is the population/timing divergence
docs/161 and docs/162 traced (the port fields ~30 % more type-1 enemies and holds `>5` entities in 294 samples where
shipped holds 3).  Four rows are unfilled port fields with a named missing caller (`fn_04933a`) or an unidentified
producer.  Only the three disjoint rows and the reachable-set width need fresh causal work.

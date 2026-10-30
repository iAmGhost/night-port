# 172 — The residual state divergences, listed and attributed, on the calibrated route

`/tmp/rig/g20` is the first full route run at the docs/168 clock calibration (`NIGHT_RIG_STEP_CALLS=665`).
It produces the current work list, and it closes one old item.

## 1. Closed: the room-record counter

`DS 0x63e14` (room-script record counter) is **not in the DIFF set** on this route.  Both sides run the same
series over the same room:

```
port    : 0 -> 88 -> 67 -> 66 -> 65 -> 64 -> 62 -> 61 -> 67
shipped : 0 -> 88 -> 67 -> 66 -> 64 -> 62 -> 61 -> 0
value census  port  {67:834, 0:355, 61:128, 88:96, 64:57, 66:21}
value census orig   {61:184, 67:95, 0:78, 88:31, 64:17, 66:12}
```

docs/153's residual ("the port holds 61 in 494 samples while shipped is never observed below 67") is superseded:
shipped reaches 61 on this route and holds it 184 samples.  The earlier mismatch was route- and cadence-dependent,
not a port defect in the drain.  Only the post-drain behaviour differs (the port re-fills to 67, shipped drops to 0
at that point), which is a phase difference rather than a count difference.

## 2. The same-width value divergences that remain, with their port-side meaning

| DS cell | first difference | port-side name and role (read from `f.c`) |
|---|---|---|
| `0x51c98` | orig `0x01` vs port `0x02` | `g_dat_51c98` "collision mode flag: 1=reinit->2, 2=active"; the port's reinit step at `f.c:9625-9626` moves 1 -> 2, so the port is already *active* where shipped still reads *reinit* |
| `0x53100` | orig `0x03` vs port `0x02` | CONTINUE credit count (docs/134 §6, docs/135) -- the port has spent a credit because its player died |
| `0x53451` | orig `0x04` vs port `0x00` | `g_dat_53451` "file-structure flag", written `(x & 0xf8) | 0x4` at `f.c:5688` (guest 0x4933d-56) -- the bit is set on shipped, not yet on the port at this sample |
| `0x53640` | byte 2: orig `0x00` vs port `0x10` | a *limit* consumed by `f.c:3441` (`if (t < g_dat_53640) t = g_dat_53640 & ~1u`, guest 0x43202-43208) |
| `0x53c48` | orig `0x71` vs port `0x2a` | CD-selection block (with `0x53c40`, `0x53c44` flagged DIFF-SIZE for lack of a shipped span) |
| `0x53c64` | orig `0xe0` vs port `0x74` | entity-list bookkeeping (with `0x53c68` orig `0x01` vs port `0x00`, `0x53c6c` 0 differing bytes) |
| `0x53c88` | orig `0x00` vs port `0x98` | `g_gauge_other_off` -- DS-relative heap offset of the gauge's other entity (docs/89) |
| `0x63e20` | 0 differing bytes, still flagged | drain-loop scratch, non-zero only inside a drain (docs/153) |

Width/size rows are separate and are oracle-basis problems, not gameplay claims: `0x53818` 4 B guest vs 256 B port,
`0x53c58` 2 vs 4, `0x53c5c` 4 vs 8, `0x53c84` 4 vs 8, `0x53e9c` 4 vs 256; `DIFF-SIZE` rows with "no shipped span"
at `0x51c24`, `0x530bc`, `0x53c40`, `0x53c44`, `0x53c4c`, `0x63f14`, `0x63f28`.
Nine pointer-cell `REC-DIFF` blocks and one `REC-NULL` entity slot remain on the record side.

## 3. How the list splits for work

- **Death-driven, one cause**: `0x53100` and the `0x53c64/68/6c` bookkeeping rows are downstream of the spurious
  player death docs/135 identified, whose `+0x30` flag came from a reused block.  Fixing the arena lifetime issue
  should move several rows at once.
- **Ordering, not arithmetic**: `0x51c98` and `0x53451` are flags whose *writer* is wired in the port but whose
  observed value is one stage early/late -- the classic "which code ran first" question, and both are cheap to
  settle with a witness at the writer against the sample index where the mismatch starts.
- **Heap-offset semantics**: `0x53c88` compares a DS-relative offset against shipped's own value; since the port's
  arena is its own (docs/134 §2), this row needs a relative comparison before it means anything.
- **CD block**: `0x53c48` plus the span-less `0x53c40/0x53c44` stay in the music cluster (docs/131, docs/114).

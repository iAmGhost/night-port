# 250 — Item 2b: no reaction record is a kind-1 record, and the only kind-1 constructor in the port runs 7 times -- so the 56 "restarts" are not constructions

Finishing docs/249's open read: the kind/type arguments at every `fn_010efc` call site, extracted by paren
matching rather than by line windows ✓

| call site | shipped addresses | type, kind |
|---|---|---|
| `f.c:14473`, `f.c:14485` (`fn_011827` reaction spawn, `x +- dx`, `y +- dy`) | 0x1186a-70, 0x11884-87, 0x118b5-ba | **0xfa, 2** |
| `f.c:15393/15396/15399` (`fn_0113e7` three-way spawn, `x-0x0c`, `x-0x2c`, `x+0x16`) | 0x11428-4d, 0x1147a-a2, 0x114cf-f7 | **0xfa, 2** |
| `f.c:15406/15409/15412` (mirror set, `x+0x0c`, `x+0x2c`, `x-0x16`) | 0x11552-71, 0x115af-d7, 0x11603-29 | **0xfa, 2** |
| `f.c:24984` (`fn_013c4d` effect actor) | 0x1415e-0x141b0 | 0xc9, 2 |
| `f.c:5604`, `f.c:27817` (player) | 0x37733, 0x3b007 | 0, 0 |
| `f.c:27007`, `f.c:27049` (`e20f_spawn`) | (0x3e20f family) | **type from the spawn record, kind 1** |

So **every reaction/effect record is kind 2** ✓ the port's dump-side kind-1 population is not secretly reaction
entities ✓ docs/249's first branch is closed ✓ (and with it the idea that docs/246 §2's per-slot hit counts were
inflated by reaction records ✓)

Combined with docs/249's other fact -- `NIGHT_SPAWNTRACE` is uncapped and printed **7** `e20f_spawn` calls in the
300 s run ✓ -- the two numbers cannot both be literal:

* docs/248's restart census says `word[record + 0x23]` jumps back up by >= 10 **56 times** in that run ✓;
* the only shipped store that can raise `+0x23` is `fn_010efc` 0x11080 (a construction ✓), and the port's only
  kind-1 construction site ran 7 times ✓

**Resolution to test next (not yet measured):** the displacement census is blind to **bulk record copies** ✓ A
record refreshed by a block copy (`mem_copy` / `fn_04192e` / the heap record copy primitives, or a
`memcpy(dst, src, 0x160)`-style template fill ✓ which is exactly how `.chr` rows become record fields elsewhere in
this port ✓ docs/237 §2's `.chr` column loads ✓) writes `+0x23` without any instruction carrying a `+0x23`
displacement ✓ So either (a) the port re-initializes a live enemy record from its type row through such a copy,
or (b) the restart count is an artifact of the record-slot hand-off (a slot switching between a kind-1 record and
a kind-2 record, then back, so the *next* kind-1 sample shows full `+0x23` again) ✓

Measurement that separates them (specified, one run): inside `fn_010efc`, count constructions by
`(kind, type_idx)` and stamp each new record with a monotonically increasing construction id stored in an unused
record field... no -- the honest version is to *not* touch the record: keep a host-side map from arena offset to
the last construction id seen at that offset, and have `hit_rate.py` restart-detection consult it ✓ i.e. compare
`restarts` (56) against `constructions at kind 1` (expected 7) **per slot** ✓ If a restart happens at a slot with
no construction, the census is a slot hand-off artifact ✓ and docs/248's "6x replacement rate" claim falls ✓

Until that check is done, the quotable item-2b facts stay: the port never writes `+0x30 = 1` on a kind-1 record
(0 in two runs ✓), the shipped does (31 samples ✓), the port's `+0x23` floor is 21-29 vs the shipped's 0-7 ✓, and
neither the hit gates, the damage model, the award table, the movers, nor the walk-step table is at fault ✓

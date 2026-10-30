# 186 — Item 2b from the record oracle: the award field differs 5 vs 53/54, and the route has no type diversity

`scripts/rig/exp_by_type.py` (new) reads the award field the shipped kill site uses —
`*(player + 0x52) += victim[+0x32]` at guest `0x15453` (`f.c:7938`) — out of every entity record block on both sides
and groups it by the record's type byte `+0x00`.  Run `g33` (canonical 150 s `ep1_kill3`, port 10 Hz / 1 500 samples,
shipped 4 Hz / 413 states).

## 1. A gate was needed before any number could be quoted

The shipped dump writes a 352-byte block at **every** pointer cell (64 per state, `timeline.json`'s `ptr_cells`), and
most of them are tables, not entities.  Ungated, the census produced shipped "types" `0x03`, `0x05`, `0x0a`, `0x22`
with awards like `0x2fdc002c`, `0x65656566`, `0x80808080`, `0x45454545` — ASCII and pointer fragments, i.e. the same
"read the wrong blob" class as docs/167 §19.  The tool now applies the docs/125 layout as an admissibility test
(`type < 0x40`, `state = dword[+0x17] < 0x100`, `hp = word[+0x29]` in `1..0x2000`), the same gate
`diff_state.py` uses.

The port side needs its own fix too, and it was a rig bug rather than a game fact: all twelve `ent##` meta entries
share the DS address of the pointer cell they were reached from (`0x53c70`), so iterating `CellStream.cells` (keyed by
DS) silently collapsed 12 record slots into 1.  The census must iterate the `meta['cells']` list, one entry per slot —
with the bug in place the port looked like it had a single entity.

## 2. The measurement

| side | admissible record samples | award `+0x32` values | nonzero |
|---|---|---|---|
| port (12 slots × 1 500 samples @10 Hz) | 1 101, **all type `0x00`** | `{0x0, 0x5}` | 174 |
| shipped (64 blocks × 413 states @4 Hz) | 711, **all type `0x00`** | `{0x0, 0x5, 0x35, 0x36}` | 573 |

Two separate conclusions:

1. **The award magnitude diverges.**  Shipped's records carry `0x35`/`0x36` (53/54) in the field the kill site adds
   into `player[+0x52]`; the port's records carry `0x5`.  That reproduces the earlier single-value observation in
   docs/125 §5 (`+0x32` shipped `0x36` vs port `0x5`) as a *population* statement rather than one sample.
2. **The route has no type diversity.**  After the gate, both sides show only type `0x00` records.  So the playtest's
   request — "check for each enemy gains different EXP per enemy type" — is **not decidable** on this route: the rig
   never observed two different entity types on either side.  That is a rig gap, not a port verdict, and it is the
   thing to fix before claiming anything per-type.

## 3. One calibration caveat that must be settled before the 5-vs-54 gap is called a defect

docs/124 recorded an offset-by-one warning on live record values (state read at `+0x18` rather than the
constructor's `+0x17` on one side).  If the shipped block and the port block are aligned one byte apart, then
"shipped `0x35/0x36` at `+0x32`" and "port `0x5` at `+0x32`" may be the same field read at different offsets.  The
test that settles it, using data already on disk: dump `+0x2c..+0x38` of an admissible record on both sides of the
same *named* entity (the kill witness `NIGHT_HITTRACE` names the victim, and the shipped side's record block is
identified by the same walk the oracle uses) and compare the byte window, not one dword.

## 4. Next actions for this item

- A drive (or `NIGHT_WARP_ENTITY` family subset) that puts **several enemy types** on screen and kills each, so the
  per-type table can actually be compared.  docs/132's family wiring already exists; the missing part is killing them.
- The byte-window calibration above, before any fix to the award path.
- Item 2a (garbled EXP text) stays open: the number the HUD prints is `player[+0x2b]` via `fn_0137c1` (a 5-glyph
  field that `fn_0137c1` itself ramps while `+0x52` drains, docs/125 §5), and the screen-side comparison of that
  region has not been run yet.

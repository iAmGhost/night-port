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

## 5. The byte-window calibration, done (docs/124's caveat answered)

Same run, admissible records, the whole window instead of one dword:

```
PORT  t=193.5 s  type=00  win +0x2c..0x38: 00 00 00 00 00 01 05 00 00 00 00 00 00 00   hp=03e8 state=0x15
SHIP  blk12      type=00  win +0x2c..0x38: 00 00 00 00 00 02 35 00 00 00 36 00 00      hp=03e8 state=0x34
SHIP  blk12      type=00  win +0x2c..0x38: 00 00 00 00 00 01 36 00 00 00 36 00 00      hp=03e8 state=0x35
SHIP  blk12      type=00  win +0x2c..0x38: 00 00 00 00 00 00 36 00 00 00 36 00 00      hp=03e8 state=0x36
```

The offset-by-one worry is answered: `hp` is `0x3e8` at the same offset on both sides, and the shipped record carries
its award value at **two** offsets (`0x36` at `+0x32` and `0x36` at `+0x36`), not at an offset shifted by one.  The
port carries `5` at `+0x32` and `0` at `+0x36`.  So the divergence is record *content*, not a mis-read dword, and the
`+0x31` byte (1/2 on shipped, 1 on port) sits where the shipped record also changes.

What stays open is instance identity, not alignment: the two sides' admissible records are in different states
(`0x15` port vs `0x34/0x35/0x36` shipped), so this is a value comparison across comparable-but-not-identical records.
Fixing the award therefore needs the shipped **writer** of `+0x32` and `+0x36` named (a disassembly question, not a
dump question), which is the next action for this item.

## 6. The shipped writers of `+0x32` (decoded, boundary-checked)

`disasm.py --search ... --all` (the `--all` flag matters: pattern hits that are not on an instruction boundary are
listed as decoded instructions, per docs/166 §15):

```
mov dword ptr [eax + 0x32], 5      0x15054  fn_01441e
mov dword ptr [eax + 0x32], 0x2e   0x335c1  fn_03272f
mov dword ptr [eax + 0x32], 0x2d   0x335cf  fn_03272f
mov dword ptr [eax + 0x32], edx    0x15070  fn_01441e
mov dword ptr [eax + 0x32], edx    0x3202f  fn_031f5f
mov dword ptr [eax + 0x32], edx    0x35cf6  fn_035c5c
```

So `+0x32` has **six** writers on the shipped side, including one that stores the literal `5` (inside `fn_01441e`, the
same hit-resolution function that carries the award site) and two in `fn_03272f` that store 46 and 45.  The
"shipped 53/54 vs port 5" statement is therefore a statement about *which writer ran*, not about a constant the game
uses for EXP: the port reproduces the `5` store, while the shipped records observed in §2 were last written by one of
the dynamic `= edx` sites.  Before any fix, the per-type question has to be re-run with the writers separated -- the
award operand and the `+0x32` scratch field may not even be the same quantity at the same moment.

## 7. Who reaches the two dynamic writers

```
branch_xrefs.py --target 0x31f5f   ->  2 edges: CALL from fn_012616 at 0x12837, CALL from fn_012616 at 0x129a7
branch_xrefs.py --target 0x35c5c   ->  1 edge : CALL from fn_01441e at 0x150f5
```

`fn_035c5c` is called from **inside `fn_01441e`** -- the same function that stores the literal `5` at `0x15054` and
`EDX` at `0x15070` -- so the two values live on one code path: a record passes through `+0x32 = 5`, then `+0x32 = edx`
via `fn_035c5c` at `0x150f5`, all within the hit-resolution function.  `fn_031f5f` is reached only from `fn_012616`
(two sites).  That is the shape the per-type census has to separate: the port's `5` is not a rival of shipped's
`53/54`, it may be the *earlier* value on the same path, overwritten before the kill award reads it.

Next measurement for this item, with existing tools: read `+0x32` across **consecutive** samples of one named record
through a kill, on both sides, and print the transition sequence -- a value timeline distinguishes "the port never
writes the big value" from "the port writes 5 and stops".

## 8. The value timeline settles the shape of the divergence

`+0x32` per slot/block, changes only, same run (`g33`).  Port (10 Hz):

```
ent00  ... 188.6s:0x0  ->  193.5s:0x5
ent02  ... 180.5s:0x0  ->  209.7s:0x5
```

Shipped (4 Hz, block index = the order `timeline.json`'s `ptr_cells` lists):

```
blk12  g_player_ent  0042:0x35  ->  0043:0x36  ->  0176:0x5
blk53  ent01         0042:0x35  ->  0043..0153:0x36  ->  0176,0207,0265:0x5
blk52  ent00         0083,0102:0x36  ->  0238,0268:0x5
blk9   g_dat_53c70   identical to blk52      blk10 g_dat_53c74  0045,0052,0082:0x36
blk54  ent02         0045,0052,0081:0x36  ->  0194:0x5
```

So the port's `5` is **not** a rival of shipped's `53/54`: shipped itself becomes `0x5` later in the same record's
life (`0x35 -> 0x36 -> 0x5`).  The port writes the `5` and never the `0x35/0x36` that precedes it.  That is the
precise statement item 2b needs: **the writer that produces 53/54 is missing or never reaches the record in the
port**, while the writer that produces 5 (`fn_01041e`'s literal-5 store at `0x15054`... cited as `fn_01441e`) is
present on both sides.

Two side observations that matter for the tooling, not the verdict: `blk9` (`g_dat_53c70`) and `blk52` (`ent00`) carry
identical timelines, i.e. the same record is reached through two pointer cells (docs/112's alias family), and shipped's
`g_dat_53c74` block holds `0x36` too -- so a census that de-duplicates by value would under-count, and one that
doesn't will double-count the same record.

Next: disassemble the region that runs before `state 0x34/0x35/0x36` is reached (`fn_01441e`'s path, plus
`fn_031f5f`/`fn_035c5c`) and find which of them writes 53/54 -- none of the six decoded stores used an immediate of
53 or 54, so the value arrives through `edx`, i.e. from a register the port may compute differently or not at all.

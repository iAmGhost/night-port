# 102 — State oracle: one generated table, two producers, per-variable per-window diff

`rig-runtime` → `rig-state-oracle`. Continues docs/101 (replay + determinism). The oracle's point is
that **neither side decides what to report**: the table of compared variables comes from the shipped
Ghidra symbol database, so the port cannot hide a divergence by not dumping it, and the original is
read as raw memory, so the port cannot invent one by naming it.

## 1. One table, generated from the shipped DB

`scripts/audit/gen_headers.py` now also emits:

- `gen/rig_cells.json` — 243 DS cells: every port global whose comment claims a DS-group address, with
  `guest_size` (from the shipped symbol / access width) **and** `host_size` (from the port's own
  declaration), plus `ptr`.
- `src/f/rig_table.h` — the same list as C, **included at the end of `f.c`** because many of those
  globals are file-scope `static` there (`g_tab_53108`, `g_rom_51acc`, …): the accessors
  `rig_cell_count()` / `rig_cell_at()` in `src/f/rig_cells.h` are the only way to name them from
  another translation unit without changing their linkage.
- `gen/rig_unmapped.json` — 6 port globals whose comment address lies outside the DS group
  (`g_dat_40010` = the stride, `g_rom_…`, `g_scale_48a36`): they cannot be read through the calibrated
  DS physical base, so they are listed instead of silently dumped. That gap is a real hole in the
  oracle and is reported as such.

Producers: `src/app/rig_sink.c` (`night --rig-dump DIR --rig-hz N`) writes one record per tick —
`t_ms, frame, audio_bytes, screen_hash, tick` in a **padding-free** layout (field order chosen so, with
a comment; the first attempt used `uint32_t` first and the reader silently read the next record's tick
as cell values, which is why every cell looked like a 0,1,2,3 counter), then the 243 cell values
(6169 B), then **64 raw bytes at each of the 43 pointer cells** (`rec_offset` in `meta.json`).
`scripts/rig/replay_original.py` mirrors it: the DS group from physical memory plus the same 64-byte
blocks read at the guest pointer values (`state_XXXX.heap.bin`).

Why raw blocks for records: the playtest items live in entity-record fields (state, stagger/lock, EXP,
HP, position, kill count). Diffing a fixed-size block against the record base localises a divergence
to `rec+0xNN` **without assuming a layout the shipped binary has not confirmed** — a wrong layout
would have produced confident nonsense.

## 2. Verdicts are explicit about what is unknown

`scripts/rig/diff_state.py` cuts the timeline into phases at the drive's key-down edges and compares,
per cell per phase, the **set of observed byte values** (not the value at a frame number):

| verdict | meaning |
|---|---|
| `SAME` | identical byte sets in every phase |
| `DIFF` | differs; the report localises it as `@DS-addr byte_orig vs byte_port` plus the port symbol |
| `REPR` | guest width ≠ port width (compares the overlap and keeps the disagreement as its own finding) |
| `SIZE?` | the shipped DB has no size for that symbol (26 cells) — comparison is only as good as the guessed width |
| `PTR` | port declares a pointer where the guest has a 4-byte field: compare presence, not value (docs/94 family) |
| `HEAP` / `ENTROPY` | allocation-address cells and the cells measured to vary across identical replays (docs/101) |
| `REC-*` | the 64-byte block behind a pointer cell: `REC-SAME` / `REC-DIFF(rec+0xNN)` / `REC-NULL` |

## 3. FP control and first real findings

`diff_state.py --baseline A B` (two identical replays of the original against each other):

```
samples: orig n=143 … | other n=144 …
tally={'PTR': 43, 'SAME': 187, 'ENTROPY': 13}
FP CONTROL (original vs original): PASS
```

`--update-entropy` *measures* variability instead of assuming it: it merged the first variable cell the
byte-level pass had missed (`0x63e1c`, a heap pointer, recorded as "varies across identical replays"),
bringing `gen/entropy_cells.json` to 24 entries.

Original vs port (`orig3` replay vs `port2` dump, same drive): `tally = {DIFF 77, PTR 43, REC-DIFF 37,
REC-NULL 6, REPR 17, SIZE? 25, ENTROPY 14, SAME 67}`. Among them the static audit's FILED-READONLY
items reappear at runtime, which is the cross-check that matters:

- `0x5274c g_tab_5274c` — original `0x3f000000` (= 0.5f, the shipped value the port multiplies by its
  zero-initialised table), port `0x0`.
- `0x52a1c g_tab_52a1c` — guest 4-byte `0x3e8` (1000) vs a 1-byte port cell (`0xe8`): width finding.
- `0x530b4` / `0x530b8` — original `0x64` (100) from the moment the anchor fires, port `0x0` for the
  whole route.

## 4. Known limitation: the two clocks start at different game milestones

The original's `t=0` is the docs/101 anchor (first framebuffer that stops changing); the port's
`t=0` is process boot, so early phases compare different game situations — most of those 77 DIFFs are
offset, not bugs. The fix is symmetric anchoring, and one half of it is already wired: the port now
emits `rig_marker(t,"res %s")` for every resource lookup (`main.c reslog_add`), and its script format
supports `wait res NAME`, which rebases the rest of the timeline by the slip.

What the *original* side cannot use, measured: scanning DS for name-shaped ASCII is **not** an event
signal — the whole resource-name table is resident (the scan finds `title4.pcx`, `boss4.act`,
`22.voc`, … all at t=0). So the recipe is `anchor static` on the emulator ↔ `wait res <title res>` in
the port script, with the residual printed by both tools.

## 5. Reproduce

```sh
python3 scripts/audit/gen_headers.py                 # -> gen/rig_cells.json + src/f/rig_table.h
(make app)                                # rig_sink linked into build/night
./build/night --res-dir ./NIGHT --script scripts/rig/drives/title_basic.txt \
     --rig-dump /tmp/rig/port3 --rig-hz 10 --no-audio --frames 2000
RUN_GAME=1 bash scripts/rig/bringup.sh && sleep 12 && python3 scripts/rig/calibrate_ds.py
python3 scripts/rig/replay_original.py --drive scripts/rig/drives/title_basic.txt \
        --out /tmp/rig/orig4 --seconds 30 --hz 3
python3 scripts/rig/diff_state.py --baseline /tmp/rig/orig1 /tmp/rig/orig2 \
        --drive scripts/rig/drives/title_basic.txt            # FP control, must PASS
python3 scripts/rig/diff_state.py --orig /tmp/rig/orig4 --port /tmp/rig/port3 \
        --drive scripts/rig/drives/title_basic.txt --md scripts/rig/reports/state_diff_title.md
```

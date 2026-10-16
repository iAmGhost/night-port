# Plan

Approach: **function-to-function** — every function in `NIGHT.EXE` is
analyzed, ported 1:1, and verified individually. Method, layout, and
verification policy: `docs/12-function-map.md` §3.

# Analysis (format & asset level — done before the pivot, still valid)
- [x] Executable format & toolchain (MZ+LE 32-bit, Watcom C/C++, DOS/4G) → `docs/01-executable.md`
- [x] Container: `NIGHT.RES` format, 444 assets → `docs/02-container.md`, `scripts/extract_res.py`
- [x] Graphics & data formats decoded byte-exact (`.pcx` `.spr` `.act` `.stg` `.wal` `.chr` `.key`) → `docs/03-formats.md`
- [x] Sound: VOC-style 8-bit mono samples, SB16 DMA ch1, sample→event map → `docs/04-sound.md`
- [x] CD-DA: MSCDEX INT 0x2E, 14 tracks → `docs/06-cdda.md`, `scripts/cdda2wav.py`
- [x] Behavior notes from playtesting (boot/menu/stage/controls) → `docs/05-logic.md` (superseded by function-level analysis where they conflict)

# Phase 0 — Inventory
- [x] Full inventory: 418 functions decompiled to `analysis/pseudo/`, `functions.tsv`, `callgraph.tsv` (3,478 edges), `rank.tsv` (bottom-up levels) → `ghidra_scripts/DumpAll.java`
- [x] Function map + zones + porting discipline → `docs/12-function-map.md`

# Phase 1 — Naming & subsystem partition
- [ ] Identify the 20 uniform large functions (0x18e88–0x2fe1e)
- [ ] Identify the isolated 39-fn cluster (0x461cc hub) and its invocation path
- [ ] Global-variable map per subsystem (Ghidra audit)
- [ ] Find indirect dispatch/function-pointer tables invisible to the direct call graph
- [ ] Seed `docs/12-functions/STATUS.md` from `analysis/rank.tsv`

# Phase 2 — Per-function port (in rank order, L0 → L19)
See `docs/12-functions/STATUS.md` for per-function status.

# Phase 3 — Integration verification
- [ ] Deterministic input script + per-frame framebuffer capture
- [ ] Pixel diff vs DOSBox-staging runs until zero (or documented diffs)

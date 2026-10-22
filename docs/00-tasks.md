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

Current position: **473/528 ported + tested, 3 skipped as non-game entries, 52 pending**
(`python3 scripts/status_report.py`; suite 2170 checks, 0 failures, ASan clean).
- Tiers closed: L0-L4, L7-L10, L15 (all stage lifecycle over shipped `NIGHT.RES`).
- Last ports: `0x01348f` the HUD gauge draw + `0x03801b` the one-frame HUD composite (room background + entity sweep +
  stage glyphs + player gauge/countdown + the 64000 B push into the VGA window), `0x012efa`/`0x03203d` the entity
  sprite draw and its sweep, `0x03c231` interlaced fill blit, `0x03b953`/`0x03b718` the floating number display.
- `0x042044` / `0x042046` / `0x0420bc` are the Watcom DOS-extender entry thunk + startup block, not game
  code — recorded `[-]` with disasm evidence in `docs/16-non-game-image-entries.md`.
- Biggest remaining levers (all callees already ported): `0x013908` 420 B, `0x03ed6f` 154 B (needs one more callee),
  `0x0113e7` 1087 B, `0x03a224` 879 B, `0x03f15f` 606 B (VESA mode probe/set — needs an SDL-side substitute),
  `0x017f13` 3956 B / `0x038f3d` 3269 B, `0x0461cc` 5909 B (FPU engine, last L5 function; also gates the 39-fn
  cluster and `0x0473bf`).


# Known port issues (open)
- [x] Resource-reader positioning **verified correct** (an earlier "off-by-N seek" suspicion was a test
      artifact): the reader is block-buffered, backward seeks un-consume inside the block, and stale
      bytes only appear if the backing image is rewritten mid-block. Semantics + test gotcha + repro:
      `docs/13-io-reader-block-buffering.md`, `scratch/io_pos_repro.c`
- [ ] Port the remaining stubs the reader path still needs for a real CD: nothing outstanding in
      `fn_04247a` / `res_consume` / `fn_041d47`, but `fn_044807`/`fn_044848` must map onto the SDL file
      layer when the .IMG backend lands
- [x] **Real-file resource backend installed** (`sys_file_real_dir`, 1-based DOS handles) and verified
      against the shipped 51 MB `NIGHT.RES` (444 entries, real sizes/offsets, block-cached re-seeks):
      `docs/14-real-resource-backend.md`
- [x] **Real stage-entry chain**: `fn_03690b` -> `fn_0374ad` (transition: teardown + `fn_037b03` + DAT_63354
      + cfg seed) -> `fn_0376c8` (hero spawn) over the shipped container, 30 checks; required fixing the heap
      region-sizing bug (fresh regions were smaller than the request that created them) -- `docs/15-real-stage-load.md`
- [x] `fn_037347` teardown over real data works: the fault was the arena substitution's per-request region
      granularity (7,450 regions for one stage; corrupt chain), fixed by `heap_region_floor_set` (64 KiB
      floor for real loads -> 34 regions). Lifecycle now load -> transition -> spawn -> teardown
- [ ] `fn_037347` teardown over real data (frees the real chains) + the entity/AI path on real stage 1
- [ ] `heap_grow` (0x04322c) is still a stub (no longer needed for stage 1 -- region sizing fixed)
      (real data runs raise the cap with `heap_arena_cap_set`; arena grown to 32 MiB, cap default 1 MiB)
- [ ] Seedable tables with no producer inside the EXE still need the CD/data backend:
      `DAT_00052c3c`, `DAT_00052738`, `DAT_00052f08`, `DAT_00053200`, `DAT_000532f4`
- [ ] `0x03a70e` (picture slide-show) is analyzed but not portable yet: its format immediate `0x16b2`
      is unmapped both in the file and in Ghidra → needs a runtime capture
# Phase 3 — Integration verification
- [ ] Deterministic input script + per-frame framebuffer capture
- [ ] Pixel diff vs DOSBox-staging runs until zero (or documented diffs)

# Real resource backend: the port's io layer over the shipped `NIGHT.RES`

Added 2026-07-13. This is the first slice of the CD/.IMG backend task: the port now reads the **shipped
51 MB resource container** through the same chunked reader (`fn_04270d` / `fn_0366a4` / `fn_0367c9` /
`fn_04192e`) that every loader uses, instead of a test-installed memory image.

## What was added

**1. A real-file backend in the sys layer** (`src/sys/sys.c`):

```c
void     sys_file_real_dir(const char *dir);  /* DOS opens resolve to "<dir>/<name>" */
uint32_t sys_file_real_opened(void);          /* successful real opens since install */
```

* `sys_file_open` builds `<dir>/<name>`, `fopen`s it, and returns a **1-based DOS handle**; a missing
  file returns **-1**, which is the DOS error value `fn_04270d` / `fn_036853` already handle.
* `sys_file_read` / `sys_file_lseek` (SEEK_SET/CUR/END + `ftell`) / `sys_file_close` route through the
  `FILE*` when the handle is real.
* Handles are 1-based on purpose: the legacy stand-in image (handle 0) and all 2100 existing checks are
  unaffected. Multiple concurrent files work, which the single-image model never did — required because
  `fn_036853` re-opens resources by name during a stage load.

**2. A raisable heap-arena cap** (`src/f/f.c`):

```c
void     heap_arena_cap_set(uint32_t cap);
uint32_t heap_arena_cap(void);
```

The physical static arena grew from 1 MiB to **32 MiB**, but the *usable* cap still defaults to 1 MiB so
every existing test keeps its historical "arena full" behaviour. The cap is raised for real data — a
single `stage1.wal` is **835 254 bytes**, i.e. larger than the old whole arena. `heap_grow` (`0x04322c`)
is still a stub, so this knob is the current substitute for the original's DPMI growth.

## Facts verified against the shipped container (17 checks, `test_real_res_container`)

Everything below is cross-checked with `scripts/extract_res.py`, which decodes the container
independently of the port:

| check | result |
|---|---|
| open `NIGHT.RES` through `fn_04270d` | handle from the real backend, node created |
| `fn_0366a4` index decode (XOR chain) | **444** entries into `g_dat_633a0` / `g_tab_5d884` |
| `fn_036626("night1.chr")` / `("stage1.wal")` (returns the entry's size word) | **11 950** / **835 254** — matches extract_res |
| `fn_0367c9("stage1.wal")` + read | room count **4** |
| `fn_0367c9("stage1.stg")` + read | record count **88** |
| `fn_0367c9("st1s.spr")` + two dwords | **7** and **0x63** (99) |
| `fn_0367c9("night1.chr")` + 8 bytes | `67 00 64 00 …` |
| unknown name | `fn_0367c9` returns 0 |
| backward re-seek after reading | lands on the same bytes (the block-cached reader is faithful over a 51 MB file) |

The test skips cleanly when the container is not present (`NIGHT_RES_DIR` overrides the default
`../NIGHT`), so a tree without the CD image stays green.

## Container quirk worth recording: `night*.chr` carries 103 records, the loader reads 101

`night1.chr` is `2 + 103 * 116 = 11 950` bytes (a 2-byte header — the port reaches it with
`fn_036853(name, 2, 0)` — then 116 stream bytes per record). The machine code nevertheless loops 101
times:

```
00037e9a  837dec65   cmp dword ptr [ebp - 0x14], 0x65
00037e9e  0f8d64010000  jge 0x38008
```

so the last two records are never read. The port's `0x65` bound is faithful (verified in the disasm, not
inferred), so this is a container/authoring quirk rather than a port bug — but it means the entity-type
table is filled for indices 0..100 only, and types ≥ 101 keep whatever `DAT_00053e9c` had.

## Next step (not done yet)

Run the **stage load itself** against real data: `fn_03690b` (master data load) → `fn_037b03(0)` with
`DAT_00063370 = 1` → `fn_0376c8` (hero spawn) → `fn_037347` (teardown), with the arena cap raised. The
io plumbing, the index, the seek behaviour and the allocation head-room are now in place; that run is
the first real exercise of the `.wal` room recursion (`fn_03c853`), the `.stg` record walk (88 records,
kinds other than 1/5 appear in real data) and the `.spr`/`.act` loaders over real bytes.

```sh
make test           # runs test_real_res_container against ../NIGHT/NIGHT.RES
NIGHT_RES_DIR=/path/to/dir ./build/test_f1
```

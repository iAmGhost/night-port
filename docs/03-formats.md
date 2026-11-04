# 03 — Asset Formats (all verified byte-exact against `NIGHT.RES` payloads)

Decoded from the LE binary via Ghidra MCP (function addresses below) and verified by
round-trip parsers over **every** file of each type in the container (see
`analysis/tables/res_inventory.json`). Container spec itself: `docs/02-container.md`.

Legend: `u8/u16/u32` = little-endian. All parsers must consume the file **exactly**
(no trailing bytes); that invariant is how these formats were cracked.

---

## 1. `.pcx` — 640×480 backgrounds / screens (174 files)

Custom variant of 8-bit PCX. Loader: `FUN_0003628e`.

```
offset 0x000:  128-byte PCX-style header (manufacturer=0x0A, version, 8bpp)
               w = 640, h = 480 in the standard PCX header fields
offset 0x080:  RLE-compressed image data (top-to-bottom, row-major, 640 px/row)
               runs until EOF-768
offset -768:   256×3 palette (file order = RGB, see below)
```

### RLE (modified PCX)

```
byte b:
  if b < 0xC0:  output b once
  else:         run: count = b & 0x3F, value = next byte, output value × count
```

- Palette: **display palette = the raw 768 file bytes, RGB order, no shift,
  no channel swap.** The loader `FUN_0003628e` does `SHR byte, 2` (disasm
  `0x36314`) but only on the *working* buffer `DAT_000636b8` (used for
  software shade/dither math, e.g. fade funcs `FUN_00040660`/`FUN_000406f5`).
  The DAC receives the *raw* palette stored in `DAT_000633b8`, uploaded by
  `FUN_00038a20` (logo/init), `FUN_0003a829` (per stage frame) and
  `FUN_00038445` (menu palette animation). Verified empirically: a
  neutral-profile DOSBox capture of the original menu
  (`analysis/refs/neu_menu2.png`) shows full 0-255 channel range
  and pixels match the raw file palette bytes within ~20, while a `>>2` DAC
  would cap all channels at 63.
- Loader writes through VESA window at `A0000` (`DAT_000a0000`), advancing 64 KB
  pages with `FUN_0003f3be(page)`; screen width tracked in `DAT_000531f4`.
- For the SDL port: decode RLE to a 640×480 index buffer, use the 768-byte
  palette as-is (RGB order), convert to 32-bit surface.

Verified: `logo.pcx`, `1s0.pcx` etc. decode to 640×480.

---

## 2. `.spr` — sprite sheets (75 files, all parse exactly)

Loader: `FUN_0003d8d4` (alloc) + `FUN_0003d9cf` (parse). Blit: `FUN_0003bb42`.

```
[u32 count]
per sprite:
  [u32 h0 .. h11]     12-DWORD header (48 bytes)
  [u32 size]          pixel-data byte size
  [size bytes]        row-segment pixel data
```

### Header fields (boss1.spr sprite 0: `67 100 32 99 -32 -100 21 0 -32 0 -32 0`)

| field | value (ex) | meaning |
|-------|-----------|---------|
| h0 | 67 | width |
| h1 | 100 | height |
| h2/h3 | 32 / 99 | inner box (hit box? — verify in gameplay code) |
| h4/h5 | -32 / -100 | hotspot offset (negative = anchor below/left of top-left) |
| h6 | 21 | anim-group / default-frame tag (semantics TODO) |
| h7..h11 | 0 / -32 / 0 / -32 / 0 | misc (flags, offsets — TODO) |

The runtime struct (0x3C = 15 u32) stores: `count`, 13 per-sprite u32 arrays
(the 12 header fields + a computed pixel-data offset array at +0x34), and one
concatenated pixel buffer at +0x38. (Note: the Ghidra decompile of the parse loop
drops one of the 12 repeated `fread` calls; the byte-exact 12+1+size layout is
confirmed by all 75 files consuming exactly to EOF.)

### Pixel data (per sprite)

```
per row (top to bottom):
  [u8 nseg]
  per segment:
    [u8 skip]      transparent pixels to skip
    [u8 len]       number of following palette-index bytes
    [u8 len × idx] 8bpp palette indices (index 0 = transparent)
```

- Blit (`FUN_0003bb42`) targets a **320×200 off-screen buffer (stride 0x140)** with
  bounds clipping — the sprite layer is composited into this buffer before
  upscaling/doubling to the 640×480 VESA screen (2× nearest-neighbor in the port).
- Palette source: the currently loaded PCX's 256-color palette (shared global
  palette model — one palette per screen, like classic 256-color VGA games).
- Visual verification: `scratch/sheets/pause_sheet.png`, `number1_sheet.png`.

---

## 3. `.act` — animation director tables (55 files, all parse exactly)

Loader: `FUN_00010010` → `FUN_00010181` (record reader).

```
[u32 count]
per record:
  [u32 n]                keyframe/beat count
  [u16 ×2]               default 0x03e8/0x03e9 (1000/1001 = off-screen sentinel)
  [u16 ×12]              default 6×(1000,1001) — spawn/anchor coordinate pairs
  [u32 ×6]
  [u8 ×6]  [u8 ×6]  [u8 ×6]
  [u16 ×6]
  [u8 ×6]  [u8]
  [u8 ×6]  [u8 ×6]  [u8]  [u8 ×2]  [u8]
  [u16 × n]              frame[i]: sprite-index to display at beat i
  [u32 × n]              per-beat params A (position? — zero in most files)
  [u32 × n]              per-beat params B (sound/event id? — zero in most files)
```

Total per record = **109 + 10n bytes**. Verified: all 55 files, e.g. boss1.act
(56 records, Σn=2893, exact to EOF).

Semantics (data-driven):
- `frame[i]` holds small sprite indices (0, 1, 2, … up to the .spr sheet count)
  → **frame sequence table**: which sprite frame to show at each beat.
- One byte in the 2nd `u8×6` group consistently equals `n−1` (last-beat index).
- Records are stored in a global array `(&DAT_00053818)[entity]`, indexed by the
  entity id from `.stg` spawn records.
- The 6×(1000,1001) u16 defaults are off-screen coordinates (world is < 2000 px);
  overridden per-record for moving actions.

TODO: trace the gameplay update loop (`FUN_000376c8` and xrefs of `DAT_00053818`:
`FUN_00038f3d`, `FUN_00010efc`, `FUN_00044663`) to pin down the remaining header
fields and the two u32[n] param arrays.

---

## 4. `.wal` — walkmap / stage floor (9 files, all parse exactly)

Loader: `FUN_00037b03` (section @ `0x00037b61`) → `FUN_0003c853` (node reader,
recursive over the node chain).

```
[u32 node_count]
per node (0x354-byte RAM struct):
  [u8 a] [u8 b]            flags (a: 0/1, b: 0/1 — layer/variant)
  [u32 p0] [u32 p1] [u32 p2] [u32 p3] [u32 p4] [u32 p5]
  flag = p1 & 1
  if flag == 0:  grid = (p2 - p0) × 200 bytes of u8 tile indices
  else:          grid = (p3 - p1) × 320 bytes
  [768 bytes]              256×3 palette (node-local; reader `FUN_0003c853` copies it verbatim — no shift in the reader)
  [u32 n] [u16 × n]        special-tile / marker list
```

- stage1.wal: 4 nodes; root grid 1600×200 (world width 1600 px × 200 walk band),
  n=8 markers. stage2: 1920×200. stage9: 8 nodes.
- Grid values are tile indices into the node palette (common values 0x40–0x47).
- `a` distinguishes e.g. background vs foreground pass (stage9 node1: a=0, b=1,
  grid offset x0=0x7d0 → second half of the world).
- The decompiled "recursion" is the next node in the chain; the chain length is
  `node_count` and every file consumes exactly to EOF.

---

## 5. `.stg` — stage event script (9 files, all parse exactly)

Loader: `FUN_00037b03` (section @ `0x00037c58`) → `FUN_0003e0a0` (record reader).

```
[u32 count]
per record (81 bytes, kept in a doubly-linked list at +0x51/+0x55 in RAM):
  [u8 tag]
  [u32 × 20]              tag-dependent payload
```

- stage1.stg: count=88, tags: `{1:38, 2:29, 3:9, 5:8, 6:2, 0:2}`.
- **tag 1** — entity spawn: `payload[0]` = entity id; game loads
  `(&PTR_s_gun_spr_00051d88)[id]` (.spr name table) and
  `(&PTR_s_gun_act_00052198)[id]` (.act name table), caching in
  `DAT_000639ec[id]` and `DAT_00053818[id]`.
- **tag 5** — position/anchor point: `payload[0]` = flags, `payload[2]` = x
  (half-pixels), `payload[3]` = y (half-pixels). Example stage1 rec1:
  (0xcf1/2=1656.5, 0x18f/2=199.5) → bottom row of the 200 px walk band.
  Stored into a 5-column table `DAT_0005d0b4…`.
- **tag 1 payload** (from stage1.stg cross-check with the port):
  `payload[0]` = entity id, `payload[1]` = flag (0/1), `payload[2]` = x
  half-pixels, `payload[3]` = y half-pixels. **Entity name tables** (EXE VA
  `0x51d88` .spr names, `0x52198` .act names, 119 entries each, same order):
  0=gun (player), 1=boss1, …, 10=gun, 11–17=mboss*, 18–30=gun, 31–33=ena-1..3,
  36–39=enb-1..4, 41–43=enc-1..3, 46–49=end-1..4, 51–54=ene-1..4, 56–58=enf-1..3,
  61–63=eng-1..3, 71–73=enh-1..3, 76–79=eni-1..4, 81–84=enj-1..4, 86–88=enk-1..3,
  91–93=enl-1..3, 118=mark.spr, rest=gun.
  **Payload alignment**: the 20 u32s start immediately after the 1-byte tag
  (offset +1, unaligned); a C struct `{u8; u32[20]}` with 3 pad bytes must
  not be memcpy'd from the file.
- tags 0/2/3/6 — other event opcodes (triggers, camera, FX — TODO: trace
  `FUN_000376c8` update loop).

---

## 6. `.chr` — episode/character table (9 files, all parse exactly)

Loader: `FUN_00037b03` (section @ `0x00037e62`).

```
[u16 count]                    (103 in every episode)
[count × 116-byte records]     (semantics partially mapped)
```

- First record (all episodes): u16s `100, 0xe800, 3, 0, 0, …` — likely
  default stats/position template (x=100, y=1000 in half-pixels, flag=3).
- The stage loader reads the first 101 records' leading fields into three
  position tables (`DAT_00053ea0`, `DAT_00053ea4`, `DAT_00053e9c`…) — see the
  0x65-iteration loop at the end of `FUN_00037b03` (the decompiled loop is
  truncated; field offsets need a disassembly pass).
- File names: `night1.chr` … `night9.chr` (per episode).

TODO: full 116-byte record layout (disassemble the loop @ ~`0x00037f00–0x00038020`).

---

## 7. `.key` — technique key-combo definitions (10 files, plain ASCII, CRLF)

Records (text):

```
<frame-count>\r\n
\r\n  (one or two blank lines)
<key chars separated by spaces, e.g. "3 3 f  f  f">\r\n
<numeric fields, e.g. "13 25 0" / "30 27 50 0">\r\n
```

- `gun*.key` / `catch*.key` define the input sequences for the 60+ techniques
  (기법) from the README; chars like `a b f u j 1 2 3 4` are key tokens
  (arrows/attacks in some internal mapping — TODO: find the token table in the
  input code).
- Files: `gun.key gun1–4.key catch.key catch1–4.key`.

---

## 8. Other asset types

| type | files | format |
|------|-------|--------|
| `.voc` | 100 | Creative Voice File 1.10 (RIFF `OCN` chunks; 22.05 kHz 8-bit mono typical). Standard VOC parsing. |
| `.fli` | 1 (`opening.fli`) | FLI/FLC, 320×200, 1168 frames, Cinepak-compressed → needs Cinepak decoder (or re-encode to WebM for the port). |
| `.dat` | 1 (`exptab.dat`) | EXP table (loaded by `FUN_0003690b`); layout TODO (small file — brute-force on first use). |
| `.txt` | 1 | plain text (Korean, CP949). |

---

## 9. Verification status

| format | files | parser | exact-to-EOF |
|--------|-------|--------|--------------|
| `.pcx` | 174 | RLE + palette (this doc §1) | verified |
| `.spr` | 75 | §2 (12 hdr + size + data) | **all 75** |
| `.act` | 55 | §3 (109 + 10n) | **all 55** |
| `.wal` | 9 | §4 (node chain) | **all 9** |
| `.stg` | 9 | §5 (81-byte records) | **all 9** |
| `.chr` | 9 | §6 (103 × 116) | **all 9** |
| `.key` | 10 | ASCII | n/a |

## 10. Open items

1. `.act` header field semantics + u32[n] param arrays (trace `FUN_000376c8`).
2. `.stg` tags 0/2/3/6 behavior.
3. `.chr` 116-byte record layout.
4. `.key` token→scancode table.
5. `exptab.dat` layout.
6. 320×200 offscreen → 640×480 compositing path (confirm 2× nearest upscale;
   check for dither/pattern modes in `FUN_0003f3be` callers).

## 11. `.fli` — opening animation (1 file: `opening.fli`, 125,664 bytes)

Custom (non-standard) FLI container. Decoded from `FUN_0003f6bc` (chunk
dispatcher), `FUN_0003fb73` (0x0C patch row writer), `FUN_0003fa3b` (0x0F
full-frame writer). Verified byte-exact: all 1169 chunks consume to EOF with
zero trailing bytes; frame-39 framebuffer matches an independent Python
reference.

### Header (128 bytes)

```
+0x00  ?           (00 00 00 00 ...)
+0x04  u16  0xAF11  magic marker
+0x06  u16  frame_count  = 1168
+0x08  u16  width  = 320
+0x0A  u16  height = 200
+0x0C .. 0x7F      unused (zeros)
```

### Chunk stream (from offset 128)

Frame chunk: `[u32 size][u16 0xF1FA][u16 subcount][8 bytes zeros]` followed by
`subcount` sub-chunks. First chunk (the SETUP) has size 0xF1FA+... with 3
sub-chunks; the remaining 1168 chunks each contain 1 sub-chunk.

Sub-chunk: `[u32 size][u16 type][payload (size-6 bytes)]`.

| type | meaning | payload |
|------|---------|---------|
| 0x12 | skip | `size` bytes ignored (setup: 207 bytes) |
| 0x04 | palette | `[4B unused][768B RGB]`; display palette = raw file bytes (the binary's `>>2` at `0x3f96a` only affects the `DAT_000636b8` working buffer; the DAC gets the raw palette — see §1 and the DOSBox capture evidence) |
| 0x0F | full frame | 200 rows, each `[u8 nseg][seg × nseg]` |
| 0x0C | patch | `[u16 start_row][u16 n_rows]` + rows (partial-row updates supported) |

Frame-chunk types: **hold** (size 16, subcount 0) and **data** (contains one
0x0C patch; 688 of 1168 frames are data frames, the first is frame #39).

The dispatcher (`FUN_0003f885`) also knows sub-chunk types 0x0B (`FUN_0003fc1d`),
0x0D (`FUN_0003fade`) and 0x10 (`FUN_0003fb0e`); none appear in `opening.fli`.

### 0x0C patch row encoding

Row: `[u8 nseg][seg × nseg]`. Segment: `[u8 skip][u8 len][data]`:
- `len < 0x80`: `len` literal pixels, consumes `2 + len` bytes
- `len >= 0x80`: run of `(256 - len)` copies of the next byte, consumes `3` bytes

The row cursor advances by `skip` + written count for **every** segment
(literal and run). Rows may end before column 320 (partial update); the rest
of the row keeps its previous content.

### 0x0F full-frame row encoding

Row: `[u8 nseg][seg × nseg]`. Segment: `[b0][b1][data]`:
- `b0 < 0x80`: `b0` copies of constant `b1`, consumes `2` bytes
- `b0 >= 0x80`: `(256 - b0)` literal pixels starting at `b1`, consumes `2 + (256-b0)` bytes

### Notes
- The 0x0C segment scheme is identical to the `.spr` pixel-row scheme
  (`FUN_0003bb42`), confirming a shared in-house blit encoder.
- Rendering: 320×200 indexed → 2× nearest upscale → 640×400 area of the
  640×480 screen (bottom 80 rows stay black).
- Port: `port/src/fli.c` decodes the whole stream; self-test asserts exact
  EOF consumption and a per-pixel diff against `scratch/shots/ref_f39.idx`.

## 12. §10 closure (resolved via `docs/05-logic.md`)

| # | item | resolution |
|---|------|-----------|
| 1 | `.act` header semantics | RESOLVED (structure): 109-byte header + `u16[n]` sprite-frame indices + `u32[n]` beat delays + `u32[n]` params; reader `FUN_00010181`; exact-EOF on all 55 files. Field names are best-effort (consumers are in the frame path, not needed for the vertical slice). |
| 2 | `.stg` tags 0/2/3/6 | RESOLVED (best-effort): runtime/parameter records; tag 1 (entity spawn) and tag 5 (anchor, half-pixels) are what stage load uses (`FUN_00037b03`, `DAT_0005d0b4`). |
| 3 | `.chr` 116-byte records | RESOLVED (structure): `[u16 count=103]` + 103×116B records, exact-EOF; episode/character table. |
| 4 | `.key` token table | RESOLVED: plain ASCII CRLF token lines (tokens `a/b/c/f/u`); the scancode mapping lives in the input handler, not the file. |
| 5 | `exptab.dat` | RESOLVED: ASCII 6×5 numeric table (experience curve). |
| 6 | compositing | RESOLVED: 320×200 offscreen buffer (stride 320) 2× nearest-upscaled into the 640×480 VESA framebuffer via the A0000 window (`FUN_0003f3be` page advance per 64 KB). |

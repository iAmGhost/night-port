# 02 — Asset Container: `NIGHT.RES` (format fully cracked)

**Goal:** resolve which file is the real asset container, decode its format, and extract all
assets. Result: **the repo's `NIGHT.RES` is the container the game uses; format fully decoded;
all 444 assets extracted and verified.**

## TL;DR

- `NIGHT/NIGHT.RES` (50,964,894 B) is a flat archive: **444 entries, stored uncompressed**.
- The CD's `NIGHT.DAT`/`NIGHT.TGS` are the *installer's* payload archives (they contain the raw
  loose assets + DOS4GW/DOS16M loader files) — **not needed** for the port.
- Container header/dir is obfuscated with a tiny reverse-XOR chain (decoded below).
- Game resolution: **640×480** (from PCX headers).
- Extractor: **`scripts/extract_res.py`** (list/info/extract).

---

## 1. Container format (verified)

```
offset 0x000:  DWORD  count                (444 = 0x01BC)
offset 0x0004: count × 0x14 bytes          (directory, obfuscated, see below)
offset 0x0004 + count*0x14 (= 0x22B4):     data blobs, packed back-to-back, uncompressed
```

### Directory de-obfuscation (reverse XOR chain)

Decoded from `FUN_000366a4` (disassembly @ `0x00036705–0x00036730`):

```python
buf = file[4 : 4 + count*0x14]
for i in range(len(buf)-1, 0, -1):
    buf[i] ^= buf[i-1]      # previous byte is still ENCODED (loop runs backwards)
buf[0] ^= 0x20
```

> Gotcha: this is only invertible backwards. A naive forward cumulative-XOR produces garbage.

### Entry layout (0x14 bytes, little-endian)

| Off | Size | Field |
|----:|-----:|-------|
| 0x00 | 12 | `name` ASCII, NUL-padded (8.3 style) |
| 0x0C | 4  | `size` DWORD |
| 0x10 | 4  | `offset` DWORD — **absolute file offset** |

Invariants (all hold for all 444 entries):
- `offset[i] + size[i] == offset[i+1]` (packed, in directory order)
- first `offset == 0x22B4` (right after the directory)
- last entry ends exactly at EOF (`0x309A99E` = file size)

### EXE-side handling (cross-reference)

| Address | Role |
|---------|------|
| `FUN_0003690b` | master asset loader — opens `night.res`, loads all named assets (guns, catch, spr, act, 100×voc, exptab) |
| `FUN_0004270d` → `FUN_000426f2` | open file → handle (stored in `DAT_000639b8`) |
| `FUN_000366a4` | read count + dir, XOR-decode, build name table at `DAT_0005d884` (stride 0x15 = name12+size4+off4+pad1) |
| `FUN_000366a4` globals | `DAT_000633a0` = count, `DAT_000633b0` = dir buffer |
| `FUN_0004192e` / `FUN_0004247a` / `FUN_00041e6c` | fread / fseek / memcpy wrappers |

Assets are fetched **by name** (e.g. `"pause.spr"`, `"1.voc"`, `"gun.key"`) — no compression at
read time.

---

## 2. Asset inventory (444 entries)

| Ext | # | Contents |
|-----|--:|----------|
| `.pcx` | 174 | PCX 8bpp RLE, 640×480 (title, help1–35, logo, stage1–9, `na*` narrative art per episode, `XsY`/`XeY` scene/ending backgrounds, `end0001–10`, `2end0001–10`, cdinsert, levelup, gameove1–3, continue, ram) |
| `.voc` | 100 | Sound Blaster VOC v1.10 ("Creative Voice File"), `1.voc`…`100.voc` (SFX/VO) |
| `.spr` | 75 | custom sprite format — bosses 1–7, mboss*, enemies `ena-1`…`enk-4`, UI (exp, face, gage, mark, system/2, over, pause, number1–3, st1s–st9s, dust, gun*) |
| `.act` | 55 | custom animation data (frame tables) — matches boss/enemy/gun set |
| `.key` | 10 | ASCII key-mapping text — `gun*.key`, `catch*.key` (5 guns + 5 catches) |
| `.chr` | 9  | `night1.chr`…`night9.chr` — per-episode character data |
| `.stg` | 9  | `stage1.stg`…`stage9.stg` — stage/level data (header: count + records) |
| `.wal` | 9  | `stage1.wal`…`stage9.wal` — walk/pathing maps (large tile-index grids, ~835 KB each) |
| `.fli` | 1  | `opening.fli` — Cinepak FLI, 320×200×8, 1168 frames, 2/70 s/frame (~10 MB) |
| `.dat` | 1  | `exptab.dat` — ASCII EXP-level thresholds ("300 1000 4000 5000 …") |
| `.txt` | 1  | `makeres.txt` — build manifest: the 444 file names in directory order |

Episode evidence: 9 × (`.chr` + `.stg` + `.wal`) matches the 9-episode structure from the readme.

## 3. Verified extraction

```
$ python3 scripts/extract_res.py extract -o scratch/res
extracted 444 file(s) -> scratch/res      # 50 MB, 444/444, zero errors
$ file scratch/res/logo.pcx
PCX ver. 3.0 ... 8-bit 640 x 480, RLE compressed
$ file scratch/res/opening.fli
FLI animation, 320x200x8, 1168 frames, 2/70s per frame
$ file scratch/res/1.voc
Creative Labs voice data - version 1.10
```

Full inventory JSON (name/offset/size): `scratch/res_inventory.json`.

---

## 4. What are the CD's `NIGHT.DAT` / `NIGHT.TGS`?

| File | Size | What it is |
|------|-----:|------------|
| `NIGHT/NIGHT.DAT` (CD) | 51,563,830 | Installer payload archive. Contains DOS4GW + **DOS/16M** runtime strings ("DOS/16M Protected Mode Run-Time", `RUN.COM`, `LOADER3.EXE`) **and the raw loose source assets** (`1.voc` @ 0x8A8E0, `gun.key` @ 0x8ABF3, `logo.pcx` @ 0x8AD26, `night.res` ref @ 0x8ABCC, `makeres.txt` @ 0x76782D). i.e., the CD ships the *inputs* of the RES build, not the packed file. |
| `NIGHT/NIGHT.TGS` (CD) | 23,100,441 | Compressed custom T.G archive (header carries sizes ≈ 0x01607AD4/0x01607ADF; no readable names). Not required — every name the EXE loads exists in `NIGHT.RES`. |

**Decision:** `NIGHT.RES` is the single source of truth for porting assets. The only CD-only
content is the 14 CD-DA audio tracks (CUE tracks 2–15) — handled in the *CD Music* task.

---

## 5. Reproduce

```bash
# list entries (optionally by extension)
python3 scripts/extract_res.py list
python3 scripts/extract_res.py list -e pcx

# inspect an entry (offset/size + first 32 bytes)
python3 scripts/extract_res.py info 1s1.pcx pause.spr

# extract everything / a subset
python3 scripts/extract_res.py extract -o scratch/res
python3 scripts/extract_res.py extract -o out logo.pcx opening.fli 1.voc
```

---

## 6. Next (per `docs/00-tasks.md`)

1. **Graphics** — decode the custom formats: `.spr` (sprite sheet: header dwords + packed pixels,
   palette handling), `.act` (frame tables referencing `.spr` data), `.stg`, `.wal`, `.chr`.
2. **Sound** — VOC chunk layout (v1.10 "Creative Voice File"), which numbers map to which events
   (table at `PTR_s_1_voc_000525a8`), playback via SB DMA.
3. **Game Logic** — main loop, the `%ds%d.pcx` background naming, combat/EXP systems.
4. **CD Music** — extract CD-DA tracks from `TG_NIGHT.CUE/IMG`; find the MIDI/track triggers in the EXE.

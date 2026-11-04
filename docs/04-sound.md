# 04 — Sound System (VOC SFX)

Analysis of NIGHT.EXE's sound-effects subsystem. CD-DA music is covered in
`docs/06-cdda.md`. All statements cross-checked against the 100-file `.voc`
corpus in `scratch/res/` (script-verified, 0 failures — `scripts/parse_voc.py`).

## 1. Hardware & initialization

README (EUC-KR, `analysis/notes/NIGHT_README.utf8.txt`):
- 音향 (audio): **Sound Blaster / Sound Blaster 16 / SB-compatible cards**
- Win95 FAQ: if no SFX, set card **DMA channel 1 → HIGH DMA 5** (confirms 16-bit
  DMA-channel-1 usage, page register 0x83)
- Options menu: SOUND MODE toggles SFX on/off (gate `DAT_00053338`)
- BGM is CD-DA; SFX are the only card-generated audio (OPL unused for sound)

Init function `FUN_00040d09` (called from game setup):
1. `FUN_00040f4f` — base-port detection: scans 8 candidate bases, writes 0xAA
   echo probe to OPL, reads back. Result → `DAT_00053334` (base, typically 0x220).
2. `FUN_00040fb3` — chip/IRQ probe: temporarily hijacks INT vectors 10h/11h/13h/
   15h/0x72, PIC masks `out(0x21,0x53)`, `out(0xA1,0xFB)`, timer `out(0x43,0xF2)`.
3. `FUN_000411d9` — IRQ line detection (read PIC ID register after `out(0x43,0xE1)`).
4. `FUN_00040788` — allocates the DMA sample buffer: 0xFFFF bytes, page-aligned
   → `DAT_00063fb4`; physical address cached in `DAT_00063fac`.
5. Installs DMA-complete/overflow handlers as far pointers (DOS4GW) at
   0x41439/0x41486/0x414d3/0x41520/0x4156d/0x415c9.
6. Enables audio: `DAT_00063f90` (audio enabled flag).

## 2. Loading

Master data loader `FUN_0003690b` (loads all game assets from `night.res`) calls,
for each of the 100 sound names `1.voc` … `100.voc` (name table at `0x000525a8`,
pointers in 103-slot table):

- `FUN_000417f3(name)`:
  - looks up the container entry (loader `FUN_00036626` size / `FUN_000367c9`,
    `FUN_00036853` name-seek — see `docs/02-container.md`),
  - allocates `size * 2` bytes, **zero-filled** (parser over-read headroom),
  - seeks past the 26-byte header, reads `size - 26` bytes into the buffer,
  - stores the buffer pointer in the **sample table** `DAT_00053d0c[i]`
    (100 × u32, entry i = sample i+1).

`FUN_000418ee()` returns the current buffer pointer (play API takes the raw
buffer pointer, not a sample id).

## 3. VOC file format (byte-level spec)

```
offset  size  field
0x00    19    ASCII "Creative Voice File"   (NO null terminator)
0x13    7     uniform across corpus: 1a 1a 00 0a 01 29 11  (decorative;
                loader skips a fixed 26 bytes)
--- tag stream (starts at file offset 26) ---
loop:
  tag   u8
  len   u24 LE
  payload[len]
```

Tags (dispatcher `FUN_00040839`):

| tag  | meaning   | payload                                                    |
|------|-----------|------------------------------------------------------------|
| 0x00 | stop      | none — a lone 0x00, always the final byte of the file      |
| 0x01 | voice     | [u8 handler][u8 rate_idx][8-bit unsigned mono PCM × (len−2)] |
| 0x02 | retrigger | same layout; reuses previous handler, writes rate_param−1 (clamped at 0x14) — not present in corpus |
| 0x08 | reserved  | 4-byte constant `54 e9 00 01`; skipped by the game (8 blocks in 84/86/95.voc) |
| other| —         | skipped (`local_20 += len`) — none in corpus               |

Voice details (handler `FUN_0004097e`):
- **handler** (payload[0]): written to port `base+0x0C` (0x22C) as a second
  command after 0x40 (`FUN_00040948`). Corpus values: 0x83, 0x9C, 0xA5, 0xA6, 0xE9.
- **rate_idx** (payload[1]): 0→`0x14`, 1→`0x75`, 2→`0x77`, 3→`0x17`
  (`DAT_00063f9c`); **all 106 voice blocks in the corpus use 0 → 0x14**.
- **PCM**: 8-bit unsigned mono, silence = 0x80. The 0xFFFF-byte DMA buffer
  (`DAT_00063fb4`) is pre-filled with 0x80 (`FUN_00041e20`), then the first
  ≤0xFFFF samples are copied in (`FUN_00041e6c`); longer voices are chained via
  the retrigger path / DMA-complete handler.

Corpus statistics (verified):
- 100 files, all with identical 26-byte header
- 106 voice blocks (6 files contain 2 voice blocks separated by tag 0x08: 84, 86, 95.voc)
- 409,329 total PCM samples
- every file ends with the stop tag as its last byte
- every voice block has len ≥ 2

## 4. Playback path (function addresses)

```
event site (e.g. combat FUN_00038f3d)
  MOV EAX, [DAT_00053d0c + 4*i]     ; sample buffer pointer
  CALL FUN_0004185e                 ; play
      gate: DAT_00063f90 (audio on) && DAT_00053338 (SOUND MODE on)
      if DAT_00063fb8 (was-playing) → out(base+0x0C, 0xD0)   ; stop previous
      CALL FUN_00040839(buf)        ; tag dispatcher (see §3 table)
        tag 1 → FUN_0004097e        ; voice (below)
        tag 2 → FUN_00040b83        ; retrigger
        tag 8 → skip 4 bytes
        tag 0 → return
      DAT_00063fb8 = 1

FUN_0004097e(payload, len):
  out(base+0x0C, 0x40); out(base+0x0C, payload[0])     ; handler
  DAT_00063f9c = [0x14,0x75,0x77,0x17][payload[1]]     ; rate param
  n = min(len-2, 0xFFFF)
  memset(DMA_buf, 0x80, 0xFFFF); memcpy(DMA_buf, payload+2, n)
  out(0x0A, 5); out(0x0C, 0); out(0x0B, 0x49)
  out(0x02, dma_lo); out(0x02, dma_mid); out(0x83, dma_page)  ; DMA ch1 (HIGH)
  out(0x03, n_lo); out(0x03, n_hi)
  out(0x0A, 1)                                  ; start playback
  out(base+0x0C, rate_param)
  out(base+0x0C, n_lo); out(base+0x0C, n_hi)    ; timer/clock values
```

The raw-port writes (0x02/0x03/0x0A/0x0B) form the vendor's latched DMA
programming sequence for 16-bit DMA channel 1 (page 0x83 per README FAQ);
`base+0x0C` (0x22C) is the SB16 timer-1 command register.

### Sample rate

Not cleanly derivable by static analysis:
- Constant 0x49 (73) is written to port 0x0B before start. If that is an
  1193180 Hz timer period: 1193180/73 ≈ 16.3 kHz.
- Rate param 0x14 (20) as a period would be ≈59.7 kHz (impossible), so the
  param and the 0x49 are not a simple pair; the vendor protocol is idiosyncratic.

**Port decision**: decode all samples as 8-bit unsigned mono at **22050 Hz
default** (era-standard 8-bit SFX rate), expose the rate as a build/runtime
knob, and calibrate A/B against DOSBox playback of the original. The timer
constants above are recorded here for reference.

## 5. Sample → event map

Sample table `DAT_00053d0c[100]` (entry i = `i+1.voc`). Sites verified by xref
in Ghidra:

| sample(s) | play site(s) (function) | context |
|-----------|-------------------------|---------|
| 1.voc | `FUN_0001441e`, `FUN_0003f6bc`, `FUN_000113e7` | shared click/jingle + opening + cutscene |
| 51–55.voc | `FUN_0003a224` (menu), `FUN_00038f3d` (combat), `FUN_00038445`, `FUN_00039c03`, `FUN_0003a70e` | menu nav / hit SFX (52–55 = combat hits) |
| 71–80.voc | `FUN_000113e7` | cutscene/dialogue voices |

### Opening animation beat cues

`FUN_00038ae0` (boot sequence) calls:
```c
FUN_0003f6bc("opening.fli", 0, 0x46, 0x48, 0);
```
`FUN_0003f6bc` is a **frame ticker**: it seeks to `opening.fli`, reads the
128-byte file header, checks the u16 marker `0xAF11` at header offset 4
(frame count u16 at offset 6 = 1168), then per displayed frame:
1. counter++ (1..1167)
2. if counter == `DAT_00053200[k]` (u32 beat-tag table) →
   play sample `DAT_00053d0c[ DAT_000532f4[k] ]` (u8 sample-index table), k++
3. advance the FLI chunk stream via `FUN_0003f80d` (reads 16-byte frame header
   with type 0xF1FA at +4, then decodes `n` Cinepak chunks via `FUN_0003f885`)

Beat → sample map (table @ 0x00053200 / 0x000532f4; 24 live cues):

| frame | sample | frame | sample |
|-------|--------|-------|--------|
| 2     | 11.voc | 306   | 19.voc |
| 3     | 11.voc | 311   | 19.voc |
| 4     | 19.voc | 313   | 19.voc |
| 5     | 19.voc | 315   | 19.voc |
| 229   | 1.voc  | 325   | 19.voc |
| 235   | 2.voc  | 478   | 2.voc  |
| 242   | 1.voc  | 482   | 2.voc  |
| 258   | 34.voc |       |        |
| 264   | 11.voc |       |        |
| 268   | 2.voc  |       |        |
| 271   | 2.voc  |       |        |
| 273   | 18.voc |       |        |
| 284   | 1.voc  |       |        |
| 289   | 2.voc  |       |        |
| 296   | 1.voc  |       |        |
| 298   | 19.voc |       |        |
| 299   | 19.voc |       |        |

(Entries 20–26 and 31–60 of the table hold tag 0 / >0xFFF values that never
match a frame counter — dead padding; entry 26 idx 0x0B is therefore never
played.)

### Remaining samples (2–50 except listed, 56–70, 81–100)

No static xrefs to their table entries — they are played through
**runtime-computed indices** (SFX numbers stored in game data, e.g. per
technique/enemy). Mapping those requires the combat/cutscene data analysis
(`docs/05-logic.md` task). The port can assign them by A/B observation against
the original (sample names are fixed: `N.voc` = table entry N−1).

## 6. Port implications

- Decoder: trivial (skip 26, walk u8/u24 tag stream). No compression.
- Play as 8-bit unsigned mono PCM; SDL3 `SDL_InitAudio` with AUDIO_U8;
  default 22050 Hz, runtime-adjustable for calibration.
- One sample at a time semantics (the card path is single-voice, stops
  previous on new play) — but the port may mix concurrently for better feel;
  keep "latest wins" only if A/B shows audible difference.
- `stage%d.spr`-style missing-file tolerance is NOT needed here: all 100
  samples exist in `NIGHT.RES`.
- Opening animation cues: drive from the FLI frame ticker (see
  `docs/03-formats.md` / port-title-opening task) using the table above.

## 7. Verification

- `scripts/parse_voc.py` — parses all 100 files per §3 spec, checks header
  uniformity, stop-tag-at-EOF, tag-8 constancy, handler/rate distributions.
  Result: **100/100 OK, 0 failures** (106 voice blocks, 409,329 samples).
- All function addresses decompiled via Ghidra 12.1.3 on the LE-loaded
  `NIGHT.EXE` (loader: ghidra-lx-loader, `watcom:LE:32:default:watcom`).

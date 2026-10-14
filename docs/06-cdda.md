# 06 — CD-DA Music (MSCDEX)

How NIGHT.EXE triggers CD-DA music. SFX are in `docs/04-sound.md`.

## 1. Disc layout (TG_NIGHT.CUE / TG_NIGHT.IMG)

2352-byte sectors, 269,303 total. Track 1 = MODE1 data; tracks 2–15 = CD-DA
(14 music tracks). Extracted to `scratch/cdda/trackNN.wav` by
`scripts/cdda2wav.py` (44.1 kHz 16-bit stereo):

| CD track | game index | INDEX 01 LBA | length | role (verified, §3) |
|----------|-----------|--------------|--------|---------------------|
| 1 | — | 0 | data | MODE1 ISO9660 (game files) |
| 2 | 1 | 37112 | 41.5 s | opening animation |
| 3 | 2 | 40265 | 4:19 | (table-driven) |
| 4 | 3 | 59904 | 3:46 | (table-driven) |
| 5 | 4 | 77031 | 4:15 | (table-driven) |
| 6 | 5 | 96388 | 4:01 | (table-driven) |
| 7 | 6 | 114678 | 4:43 | (table-driven) |
| 8 | 7 | 136158 | 5:27 | (table-driven) |
| 9 | 8 | 160997 | 5:16 | (table-driven) |
| 10 | 9 | 184970 | 6:05 | main menu |
| 11 | 10 | 212667 | 2:16 | stage |
| 12 | 11 | 222996 | 3:55 | combat |
| 13 | 12 | 240852 | 3:55 | combat/boss variant |
| 14 | 13 | 258720 | 2:08 | (table-driven) |
| 15 | 14 | 268471 | 11 s | short outro/jingle |

## 2. Triggering mechanism (function addresses)

MSCDEX (`INT 0x2E`) is the API. No MSCDEX strings in the binary — the game
talks to it directly through DOS4GW's real-mode interrupt engine.

| function | role |
|----------|------|
| `FUN_000103e0` | real-mode INT engine (builds the 16-bit register map, executes the interrupt under DOS4GW) |
| `FUN_00010570` | MSCDEX wrapper: far-copies the param block (`FUN_00041e6c`) then calls the INT engine |
| `FUN_00010927` | **Read TOC** — AH=0x17, subcommand 0x0A. Result: track count → `DAT_00053c60` (=15), allocates `DAT_00053c64` |
| `FUN_000107b9` | **Read track data** (per track, subcommand 0x0B). Fills 5-byte entries `{status, LBA lo/mid/hi}` into `DAT_00053c64`; lead-out LBA stored just past the table |
| `FUN_00010b1d(k)` | **Play track** — AH=0x15. `k` ∈ 0..14 indexes the TOC table → plays CD track `k+1` (start LBA from table; end = next track start / lead-out). Playing a new track implicitly stops the previous one |
| `FUN_000405a7` | 16-byte MSCDEX param block (leading u16 0x0013 = "Get Current Time" per spec) — drive poll/keep-alive; result discarded |
| `FUN_00010664` | drive probe / param init (param blocks `DAT_00051a3c` etc.) |
| `FUN_00010dde` | aux MSCDEX call with 2 KB buffer (param `DAT_00051b54`); not on the main music path |

### CD-present & disc-identity check

`FUN_00010c3e` — the "insert CD" gate (shown before gameplay):
```c
do {
    FUN_00010927();                       // re-read TOC
    if (DAT_00053c60 == 0xf               // 15 tracks
        && *(u32*)(DAT_00053c64 + 0xb) - *(u32*)(DAT_00053c64 + 6) == 0xc51)
        return;                           // correct disc present
    FUN_0003f15f();                       // (re)enter VESA mode
    FUN_0003628e("cdinsert.pcx", 1);      // show "insert CD" screen once
    FUN_00035d5d();                       // wait/input
} while (true);
```
The `0xC51` (3153) check is a **disc fingerprint**: track-3 start LBA
(40265) − track-2 start LBA (37112) = 3153 frames — exactly the TG_NIGHT.CUE
values. A different disc fails the check and the screen loops.

`cdinsert.pcx` exists in `NIGHT.RES` and is extracted to `scratch/res/`.

## 3. Track → game-state mapping

Verified by decoding the `mov eax, N; call FUN_00010b1d` prologues at every
call site (15 sites):

| game track (CD track) | call site(s) | function (context) |
|-----------------------|--------------|--------------------|
| 1 (CD 2) | 0x38b02 | `FUN_00038ae0` — boot/opening sequence |
| 9 (CD 10) | 0x3a442 | `FUN_0003a224` — main menu |
| 10 (CD 11) | 0x3b2cb | `FUN_0003af18` — stage gameplay |
| 11 (CD 12) | 0x3b089, 0x391fb, 0x39a4c | `FUN_0003af18`, `FUN_00038f3d` — combat |
| 12 (CD 13) | 0x3b20a, 0x3b479, 0x38637, 0x38877 | `FUN_0003af18`, `FUN_00038445` — combat/boss variant |
| table-driven | 0x3a878, 0x10db9, 0x39e8c, 0x3d3d7 | `FUN_0003a829`, `FUN_00010d47`, `FUN_00039c03`, `FUN_0003d244` — episode music; index computed `mov eax,[eax+0x40]; shl eax,2; adc eax,edx; mov eax,[eax+0x2da4]` (music field in the episode data structure → table at 0x2da4) |

So: tracks 3–9 (CD 4–10) and 13–14 (CD 14–15) are selected per-episode via the
data table; tracks 1, 9, 10, 11, 12 (CD 2, 10, 11, 12, 13) are fixed by state.

## 4. Port implications

- No MSCDEX on modern OS → play the extracted WAVs. Mapping: **game track k →
  `scratch/cdda/track{k+1:02d}.wav`**.
- Vertical slice needs: CD 2 (opening), CD 10 (menu), CD 11 (stage),
  CD 12/13 (combat).
- "Play new track" semantics: stop previous, start at track beginning, loop or
  auto-stop at track end (MSCDEX plays to end-of-track).
- Disc-identity gate can be skipped in the port (or emulated by checking the
  WAV set exists).

## 5. Verification

- Track list & LBAs from `NIGHT/TG_NIGHT.CUE`; WAV extraction
  `scripts/cdda2wav.py` run 2026-10-14: 14/14 tracks, sizes 1.9 MB–64.4 MB.
- Fingerprint cross-check: 40265 − 37112 = 3153 = 0xC51 = the EXE's check
  value ✓.
- Function roles from Ghidra 12.1.3 decompilation (LE-loaded NIGHT.EXE);
  track numbers from call-site disassembly (`mov eax,N` immediates).

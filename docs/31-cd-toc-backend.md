# The real CD backend: TG_NIGHT.CUE/IMG answers NIGHT's MSCDEX reads (and the disc gate passes)

Added 2026-07-13 — `port/src/cd/cd_iso.c` + `cd_iso.h`, 8 new checks (`test_cd_iso_real`), suite
**2738 → 2746 PASS, 0 FAIL**.

## The open question from docs/28 is resolved: `fn_010927` reads the **TOC**, not the root directory

`fn_010927`'s 26-byte request packet copied from ROM `0x51ad4` starts `{0x1a, 0x00, 0x03, 0x00, ...}`
= length 26, unit 0, **request code 3** — MSCDEX *read TOC header*. The 7-byte reply the game shuttles
through `DAT_00053c5c` is therefore:

| byte | meaning | game use |
|---|---|---|
| `+0` | drive / unit number (ROM seed `0x0a`) | untouched |
| `+1` | **first track number** | `bufA[1]` |
| `+2` | **last track number** | `bufA[2]` → `DAT_00053c60 = last - first + 1` |
| `+3..+5` | **lead-out address, MSF** | `bufA[3] + bufA[4]*75 + bufA[5]*6420 - 0x96` written to the trailing dword — an MSF→LBA conversion whose minute coefficient is `6420` instead of `4500` (an original bug, ported as-is; `0x96` = 150 = the 2-second lead-in) |

`fn_107b9`, called once per entry, is MSCDEX *read TOC entry*, and the array `fn_010927` allocates with
stride **5** is `{BYTE trackNumber; DWORD startLBA}` per track. That explains the disc gate
`fn_010c3e` reading `recs + 6` and `recs + 0xb`: with a 5-byte stride those are the `startLBA` dwords of
**entry 1 and entry 2**, i.e. tracks 2 and 3.

## The numbers fall out of the real disc

`NIGHT/TG_NIGHT.CUE` lists **TRACK 01 MODE1/2352 + TRACK 02..15 AUDIO** → 15 tracks, so
`DAT_00053c60 = 15 - 1 + 1 = 0xf` ✓ (the gate's first condition).

`TRACK 02` carries `PREGAP 00:02:00`, i.e. 150 sectors of virtual silence that are *not* in the IMG, so
every track from 02 up sits 150 sectors later on the disc than its file position:

| track | cue INDEX 01 | file frames | disc LBA (= file frames + pregap) |
|---|---|---|---|
| 01 | `00:00:00` | 0 | 0 |
| 02 | `08:14:62` | 37112 | **37262** |
| 03 | `08:56:65` | 40265 | **40415** |

`40415 - 37262 = 3153 = 0xc51` ✓ — the gate's second condition, exactly. So the gate is a **TOC
signature check of the original pressing** (an anti-copier measure), and the shipped disc satisfies it
without any stubbing: `test_cd_iso_real` installs `cd_iso_root_header` / `cd_iso_root_records` as the
`sys_cd` backend and runs `fn_010c3e` itself — it returns after one poll, paints nothing, and never
asks for the CD to be stopped.

## Image form

`TG_NIGHT.IMG` is `BINARY` MODE1/2352 as the CUE declares, but its sectors carry the ISO9660 user data
at **offset 16** (the PVD signature `CD001` sits at sector+17, the PVD root-directory record decodes at
+156 → root extent LBA 22, size 2048; `/NIGHT` at LBA 23 holds `NIGHT.DAT` 51,563,830 B and
`NIGHT.TGS` 23,100,441 B plus the installer files). So `cd_iso_read_sector()` maps a disc LBA to
`[lba*2352 + 16, +2048)`.

## What the port still needs from this layer

CD **audio** playback (`fn_010b1d` selects tracks 10/11/12 from the game main; the emulator-free port
has to stream those tracks from the IMG using the same track LBAs) and the audio-stop seam
(`fn_000405a7` → `sys_cd_audio_stop`, already counted).

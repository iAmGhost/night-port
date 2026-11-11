# 01 — `NIGHT.EXE`: Executable Format & Toolchain Analysis

**Goal:** establish what `NIGHT.EXE` *is* (format, compiler, runtime, memory layout, entry)
before porting, so the port targets the right architecture and the RE work in Ghidra is
trustable.

**TL;DR**
- Format: DOS **MZ** stub + **LE (Linear Executable)**, **32-bit**.
- Compiler: **Watcom C/C++ (32-bit)**.
- DOS extender: **Rational DOS/4G (DOS4GW)** — its runtime is embedded in the binary.
- Entry point (Ghidra linear address): **`0x00042044`**.
- 3 memory objects: `.object1` (code+init data), `.object2` (data), `.image` (raw file overlay).
- For the port: this is a protected-mode x86 DOS-extender app (SVGA + Sound Blaster). The
  DOS4GW extender itself is irrelevant to the port — it is only the *mechanism* that let the
  1990s 32-bit code run under DOS.

---

## 1. Files in the original distribution (`./NIGHT`)

| File | Size | Role |
|------|-----:|------|
| `NIGHT.EXE`   | 311,831 (0x4C217) | The game (MZ+LE, 32-bit) |
| `DOS4GW.EXE`  | 265,420 (0x40CCC) | Rational DOS/4G extender (MZ; reference/standalone copy) |
| `NIGHT.RES`   | 50,964,894 | Packed game assets (graphics + SGI 8-SVX audio) |
| `NIGHT.BAT`   | 40     | Launch batch file |
| `NIGHT.ICO`   | 2,238  | Icon |
| `README.TXT`  | 19,283 | EUC-KR readme (title, controls, system reqs) |
| `TG_NIGHT.CUE`/`TG_NIGHT.IMG` | 633 MB | Original CD image (ISO9660 data track + 14 CD-DA tracks) |

---

## 2. DOS `MZ` stub (file offset 0x0000)

Raw (first 64 bytes):

```
000000  4d 5a 4e 00 16 00 06 00 06 00 87 00 ff ff a5 02   MZN.............
000010  00 08 00 00 0e 03 00 00 40 00 00 00 00 00 00 00
000020  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
000030  00 00 00 00 00 00 00 00 00 00 00 00 50 2a 00 00   ............P*..
```

Decoded MZ header fields:

| Off | Field | Value | Note |
|----:|-------|------:|------|
| 0x00 | `e_magic`   | `0x5A4D` | "MZ" |
| 0x02 | `e_cblp`  | `0x004E` | last-page bytes (78). **High byte `0x4E` = 'N'** → the stub reads **`MZN`**, the Rational DOS/4G stub marker. |
| 0x04 | `e_cp`    | `0x0016` | 22 pages |
| 0x08 | `e_cparhdr`| `0x0006` | |
| 0x0A | `e_minalloc` | `0x0087` | |
| 0x0C | `e_maxalloc` | `0xFFFF` | |
| 0x0E | `e_ss`    | `0x02A5` | real-mode stack segment |
| 0x10 | `e_sp`    | `0x0800` | real-mode stack pointer |
| 0x14 | `e_ip`    | `0x030E` | real-mode entry (DOS stub) |
| 0x18 | `e_lfarlc`| `0x0040` | **bit 6 set → 32-bit / protected-mode executable** (not a 16-bit MZ, not NE) |
| 0x3C | `e_lfanew`| `0x00002A50` | offset of the **LE** header |

The real-mode `e_ip`/`e_ss` point into the tiny DOS stub whose only job is to hand off to the
DOS/4G runtime, which sets up protected mode and jumps to the real 32-bit entry (below).

---

## 3. LE (Linear Executable) header (file offset 0x2A50)

Raw (first 64 bytes, for reference):

```
0002a50  4c 45 00 00 00 00 00 00 02 00 01 00 00 00 00 00   LE..............
0002a60  00 02 00 00 3f 00 00 00 01 00 00 00 44 20 03 00   ....?.......D ..
0002a70  02 00 00 00 c0 50 01 00 00 10 00 00 17 08 00 00
0002a80  18 ad 00 00 00 00 00 00 4e ae 00 00 00 00 00 00
```

- Magic at `0x2A50` = **`LE`** (Linear Executable).
- **Verified entry offset:** the 4-byte little-endian value `44 20 03 00` = **`0x00032044`**
  sits at `LE+0x1C`. With the LE image base `0x10000` this yields the linear entry
  **`0x10000 + 0x32044 = 0x42044`**, matching Ghidra's entry point exactly.

> Note on decoding: a byte-by-byte decode against the *generic* LE header template does **not**
> line up cleanly for this DOS4GW-produced file (field offsets differ from the vanilla spec),
> so the authoritative field interpretation is taken from Ghidra's LE loader
> (`ghidra-lx-loader`, "Linear Executable (LE-Style DOS)"), which imports the file correctly.
> Reproduce the exact fields in Ghidra rather than by hand (see §8).

---

## 4. Compiler & runtime identification (raw evidence)

String scan of `NIGHT.EXE` (offsets are file offsets):

| Marker | Offsets | Meaning |
|--------|---------|---------|
| `WATCOM C Run-Time system code is provided on an "as is" basis and is (c) Copyright by WATCOM International Corp. 1988-1993.` | 0x371, 0x3C7 | Watcom 16/32 C run-time present |
| `WATCOM C/C++32 Run-Time system. (c) Copyright by WATCOM International Corp. 1988-1994.` | 0x3FA46, 0x3FA77 | **Watcom C/C++ 32-bit** run-time (1994 vintage) |
| `RATIONAL DOS/4G` | 0x4A200 | **Rational DOS/4G (DOS4GW)** extender runtime |

Corroboration:
- Ghidra's language/compiler detection picks **`watcom:LE:32:default`** (compiler spec `watcom`)
  for the import — consistent with the embedded Watcom 32-bit run-time strings.
- The `MZN` MZ stub (§2) + embedded `RATIONAL DOS/4G` strings (§4) confirm a DOS/4G extender app.

**Conclusion:** written in **Watcom C/C++ (32-bit)**, linked under **Rational DOS/4G (DOS4GW)**.

---

## 5. Memory map (from Ghidra import)

| Block | Range | Size | Purpose |
|-------|-------|-----:|---------|
| `.object1` | `0x00010000 – 0x0004AB1C` | 240,413 | LE **object 1** — code + initialized data (entry lives here) |
| `.object2` | `0x00050000 – 0x000650BF` | 86,208  | LE **object 2** — data segment |
| `.image`   | `0x00000000 – 0x0004C216` | 311,831 | overlay of the raw file bytes (`.image::<hex>` addresses) |

Program facts (Ghidra): `function_count = 415`, `symbol_count = 2672`, `memory_size = 638,452`,
single 32-bit `ram` address space + `.image` overlay.

> The large gap between `.object1` end (`0x4AB1C`) and `.object2` start (`0x50000`) is normal for
> LE: objects are placed at fixed segment-granular addresses; the unused middle is not loaded.

---

## 6. Entry point & startup

- Ghidra entry point: **`_entry @ 0x00042044`** (kind `external_entry`), inside `.object1`.
- `0x00042044` is reached after the DOS stub → DOS/4G protected-mode init → the Watcom C
  start-up (`__cstartup`/`__main`), which then runs the program's `main`.
- The DOS4GW runtime init region sits just below the entry (`~0x000420BC`), consistent with a
  DOS4GW prologue. (Function-level startup/`main` identification is a separate task — see
  `docs/00-tasks.md` → Game Logic.)

---

## 7. Implications for the port

1. **Architecture:** 32-bit protected-mode x86. Not directly runnable on modern OSes; a port (or
   an emulator like DOSBox) is required. The port reimplements behaviour, not the DOS4GW layer.
2. **Extender is irrelevant to the port:** DOS/4G only provided 32-bit memory management +
   DPMI-style services under DOS. Its job (linear 32-bit memory) is free on modern systems.
3. **Hardware dependencies to replicate** (confirmed by in-binary strings, analysed elsewhere):
   - **Graphics:** SVGA via **VESA BIOS** (`int 0x10`, mode set `0x4F01`); windowed memory at `0xA0000`.
     Error string: `Your Vga Card Not Support VesaMode!!!`. Screen width held in a global.
   - **Audio:** Sound Blaster / DMA, plus **SGI 8-SVX** sample data and **CD-DA** tracks on the CD.
4. **Asset source of truth:** `NIGHT.RES` (installed) vs `NIGHT.DAT`/`NIGHT.TGS` (on CD) differ in
   size/format — resolving the exact container is the next goal (Graphics/Sound tasks).

---

## 8. How to reproduce (Ghidra)

Tooling:
- Ghidra **12.1.3** at `/opt/homebrew/Cellar/ghidra/12.1.3/libexec/Ghidra/`
- Java 21 at `/opt/homebrew/opt/openjdk@21`
- Extension **`ghidra-lx-loader`** (built from `yetmorecode` master for 12.1.3) installed under
  `~/Library/ghidra/ghidra_12.1.3_PUBLIC/Extensions/ghidra-lx-loader`
  (provides the `Linear Executable (LE-Style DOS)` loader + `watcom` compiler spec).

Headless import (verified working):

```bash
/opt/homebrew/Cellar/ghidra/12.1.3/libexec/Ghidra/support/analyzeHeadless \
  <project_dir> <project_name> \
  -import extracted/NIGHT.EXE \   # the installer payload, as `make extract` unpacks it (docs/455)
  -processor "watcom:LE:32:default:watcom"
```

Result of a successful import:
`Loader: Linear Executable (LE-Style DOS) | Language: watcom:LE:32:default:watcom |
Entrypoint set @ 00042044 [base 00010000 + eip 00032044] | Import succeeded`.

The `NIGHT.EXE` program is currently open in the running Ghidra MCP session (current program).
Query live facts with the MCP tools, e.g. `get_current_program_info`, `list_segments`,
`get_entry_points`.

Raw-header helper: `scripts/parse_le.py` (prints MZ + LE header fields + compiler markers to
`scratch/le_report.txt`).

# NIGHT.EXE segment mapping (code vs data)

Extracting bytes straight out of `NIGHT/NIGHT.EXE` needs **two different VA -> file offsets**, because
the flat DOS32A (Watcom) image is not stored contiguously:

| region | VA range (approx) | file offset |
|---|---|---|
| code + embedded strings | `0x1000` .. `~0x4f000` | `VA - 0x2600` |
| initialized data segment | `0x4f600` .. `~0x6xxxx` | `VA - 0x7600` |

## How the data mapping was pinned

The port needs the constants at DS `0x51c24` (entity type -> sound-slot index, read by `0x0113e7`).
Reading them at `VA - 0x2600` returned zeros (that file window is BSS-sized padding), while Ghidra's
image showed real values (`39, 35, 31, 27, ... 92, 93, 94, 82, 83, 84, 85, 87, 88, 89, 90`).
Searching the file for the pattern `27 00 00 00 23 00 00 00 1f 00 00 00 ...` found it at file
`0x4a624`, i.e. `0x51c24 - 0x4a624 = 0x7600`. The whole probed region (`0x51c24 .. 0x51e00`, 0x1dc
bytes) is byte-identical to Ghidra's bytes there, so the delta is the segment mapping, not a coincidence.

## Tooling

* `scratch/fdis.py` — code listings (`VA - 0x2600`).
* `scripts/extract_ds.py <va-hex> <len> [name]` — emits a C `uint8_t name[] = { ... }` initializer for a
  data-segment range through the `VA - 0x7600` mapping.

Already extracted this way (in `port/src/f/f.c`):

* `g_blk_51c24[0x1dc]` — DS `0x51c24 .. 0x51e00`, one contiguous initialized block that carries
  * `+0x00` `uint32[type]` -> sound-slot index (`0x0113e7`),
  * `+0x5c` the region previously modelled as `g_blk_51c80`,
  * `+0x6c` gauge fill colours `DAT_00051c90[]` / `+1 DAT_00051c91[]` (`0x01348f`).

  The earlier model was a zero-filled `g_blk_51c80[0x180]` whose `+0x40` stood in for `DAT_00051c90`,
  which both dropped every constant in the region and placed the colour table 0x40 bytes too high
  (`0x51cc0` instead of `0x51c90`).

## The 0x50000 data-segment base inside instructions (unblocks "unmapped pointer" blockers)

Capstone prints raw instruction operands, Ghidra prints them with the data-segment base folded in, so the
same operand looks different:

```
capstone : a1 703c0000   mov eax, dword ptr [0x3c70]
ghidra   : DAT_00053c70
```

The rule is **data-segment operands and data-segment immediates carry an implicit base of 0x50000**:

    flat VA = operand + 0x50000        (and then file = flat VA - 0x7600)

Verified against the shipped file:

| immediate | flat VA | file | string |
|---|---|---|---|
| `0x15fe` | `0x515fe` | `0x49ffe` | `help%d.pcx` |
| `0x1609` | `0x51609` | `0x4a009` | `help%d.pcx` |
| `0x16b2` | `0x516b2` | `0x4a0b2` | `na%d%04d.pcx` |

`0x16b2` had been recorded as a blocker for the picture/CG slide-show driver `0x03a70e` ("unmapped both
in the file and in Ghidra") -- it is a plain data-segment pointer, so that function is unblocked.
When a port hits an immediate below the code segment (below `0x2600`) that looks like a pointer, add
`0x50000` and read it through the data mapping before calling it unmapped.

## Caution for future ports

A DS symbol that (a) is never written anywhere in the analyzed set and (b) reads as zero in the port is
probably an **image constant in the data segment**, not BSS. Re-read it through `VA - 0x7600` (or
`scripts/extract_ds.py`) before assuming zeros — and remember that Ghidra's flat image already has those
bytes folded in, so its `DAT_...` reads can disagree with a raw file read at the code mapping.

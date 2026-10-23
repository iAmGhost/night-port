# 53 — the intro `.fli` artifact: an independent reference decode, and what the port actually dispatches

## A reference that needs no emulator

`scratch/res/opening.fli` is a genuine Autodesk FLC: magic `0xAF11`, **1168 frames**, 320x200, 8 bpp.
ffmpeg's `flic` decoder handles it, so the original's intro frames can be obtained without DOSBox:

```
ffmpeg -v error -i scratch/res/opening.fli -fps_mode passthrough /tmp/fli/%03d.png   # 1169 PNGs
```

The port's own frames come from the playtest harness (`NIGHT` shots tagged `fli` during playback).
`scratch/fli_compare.py` reduces both sides to 80x50 luma and reports the nearest reference frame per
port shot plus the worst row of the difference.

Result: the port's `.fli` frames do **not** match any reference frame (`L1 ≈ 42` with a saturated
difference on specific rows, vs `L1 = 2.0` for a pair that is genuinely identical), and a side-by-side
montage shows the port painting structure with heavy speckle where ffmpeg shows clean artwork. So the
intro artifact is a real port-side decode/present bug, not a host presentation effect. The port's frames
*do* change over time and keep the composition (same caption position), so the stream is being walked,
just decoded wrong.

## What the port dispatches while playing it

`NIGHT_FLIDBG=1` (new) logs every resource-op `fn_03f885` dispatches. Over the intro:

```
[fli] op=0x12 payload=201      1x   -> the dispatcher's DEFAULT arm (payload discarded)
[fli] op=0x04 payload=772      1x   -> palette: skip 4, read 768 B, >>= 2, board_out(0,0x100)
[fli] op=0x0f payload=1400     1x   -> fn_03fa3b (200-row count-prefixed RLE)
[fli] op=0x0c                  87x  -> fn_03fb73 (per-row packet RLE: skip / copy-or-fill)
```

So 87 of the ~90 blocks are op `0x0c`, which ffmpeg's equivalent decodes as **WORD_RUN (chunk type 12)**
— where run lengths are counted in **2-byte words**, while `fn_03fb73` advances the destination by the
packet's skip byte and copies `ctl` **bytes**. That is the prime suspect for the speckle: a byte-unit
reading of a word-unit stream drifts the destination within the row and misreads every following packet.
Op `0x12` (payload 201) is routed to the default arm and simply discarded — harmless if it is a
FLI_SMALL_COPY/palette-style op the game ignores, worth confirming against the reference decoder.

## Next step (queued)

Decode a single `0x0c` block in isolation: feed the file's first such block through `fn_03fb73` from a
unit test (the resource reader can wrap a memory buffer) and compare the produced 320x200 rows with the
matching ffmpeg frame. Then re-run the montage/`fli_compare` numbers; they are the objective pass/fail
for the `logo-intro` task's second half.

## Fixed: `fn_03fb73` (op 0x0c) lost the EDI advance, so every packet after the first re-wrote the same pixels

The reference chain above (op 0x04 palette + op 0x0f first frame + 687 op 0x0c deltas) reproduces
ffmpeg's frames at **edge correlation 1.000** (luma L1 ≈ 0.3/255), so it is a valid ground truth. Feeding
it the *same bytes the port sees* (verified: `NIGHT_FLIDUMP` payload dumps are byte-identical to the file's
blocks) exposed the divergence, and disassembly of 0x3fbd6-0x3fc09 explains it:

```
0003fbe7  6689c1   mov cx, ax
0003fbea  f3a4     rep movsb byte ptr es:[edi], byte ptr [esi]   ; COPY: EDI advances by CX
...
0003fbf4  89c1     mov ecx, eax
0003fbf7  f3aa     rep stosb byte ptr es:[edi], al               ; FILL: EDI advances by CX
```

`rep movsb` / `rep stosb` advance the destination; the port's `memcpy` / `memset` did not advance `d`, so
each row's second, third, … packets overwrote the first one's pixels. With the advance restored, all 88
port snapshots of the decode buffer are **byte-identical** to the reference decode:

```
compared 88 port index-buffer snapshots against the reference decode
mismatching snapshots (after the first): 0 []
```

Regression test: `test_fn_03fb73` T3 (three packets in one row: copy 2, skip 1 + fill 3, copy 2) now
asserts the writes chain — `[0..1]`, `[3..5]`, `[6..7]` — instead of stacking on `[0..]`.
2810 checks ALL PASS.

## Closed: the 320x200 movie had to drive the host view, and the presented frames now match the reference

The decode target `g_dat_51a0c` (= the port's video arena) is written with a **320-byte pitch**, and the
screenshots taken during the movie are **640x480**, so the picture the harness shows is the 320-pitch
movie read back with a 640 pitch — sheared into the speckle the report calls an artifact. The decode
itself is now provably correct, so the remaining defect is on the presentation side.

Constraints found while looking for the mode switch:

* the EXE contains exactly **one** VESA set-mode call, `0x3f36e` with `BX = 0x101` (640x480), inside
  `fn_03f15f`, which the opening step calls *after* the player (the existing test pins that restore);
* the player `fn_03f6bc` never touches the mode, and the ops it dispatches are only 0x12 (payload 201,
  the Autodesk FLI_MINI thumbnail — correctly discarded), 0x04 (palette), 0x0f, 0x0c; there is no
  per-frame "present" op (0x0d/`fn_0003fade` is never reached during the intro);
* so the movie lives in a 64000-byte 320x200 work screen and must reach the 640x480 screen through the
  scaler/blitter family — `fn_03f462` (seen blitting `(0,0,0x280,0x1e0,scratch)` for the title wipe) and
  the frame-pitch global `DAT_00040010` (`fn_040631`) are the two candidates, together with the port's
  `g_view_320x200` view flag (set today only by the room-frame flip `fn_038098`).

Resolution: the game has no second mode set, and every CG image op addresses a **320-pitch, 64000-byte**
frame, while the still-image loaders cover 307200 bytes (640x480). So the *view* is per-step, and the
port's host-side view flag (`g_view_320x200`, set today only by the room-frame flip `fn_038098`) simply
was never raised for the movie: a 320-pitch frame was read back with a 640 pitch and sheared into speckle.
`fn_03f6bc` now brackets the play with the 320x200 view (saving/restoring the caller's value).

Measured afterwards -- the port's presented `.fli` screenshots against ffmpeg's reference frames, 80x50
luma, best-matching reference index per shot:

```
0008_fli.bmp  best ref # 39  L1= 2.00
0009_fli.bmp  best ref # 57  L1= 2.00
0010_fli.bmp  best ref # 68  L1= 1.51
0012_fli.bmp  best ref # 98  L1= 1.45
```

`L1 ≈ 1.5-2.0` is the floor an *identical* pair produces here (the same value the known-matching pairs
give), and the matched indices advance through the movie, so the intro now tracks the reference frame by
frame. Montage: `docs/manual-playtest/02-shots/05-fli_port_vs_ffmpeg.png` (port x4 | ffmpeg x4).

Regression pins added: `fn_03f6bc` runs with `g_view_320x200 == 1` (observed by the resource lookup it
performs, via the new `g_res_view_flag` instrumentation) and restores the caller's value afterwards; plus
the `fn_03fb73` packet-chain test from the fix above. 2812 checks ALL PASS.

Remaining for this task: the logo half of the contract still wants the *original's* logo frame from
DOSBox (the port side is compared against the extracted `logo.pcx` and the fade profile today) -- that is
the `docs/46` capture one-liner, which needs an interactive terminal.

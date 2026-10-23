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

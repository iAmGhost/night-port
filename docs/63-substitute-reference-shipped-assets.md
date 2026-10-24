# 63 — The reference that replaces the original's frames

The playtest contracts ask for port screenshots "compared against the same moments captured from the original
game in DOSBox". That capture half is a measured dead end on this machine (all five routes, docs/46). This
document records the substitute reference and exactly what it can and cannot prove, so the remaining
comparisons are made against a stated basis instead of silently relaxed.

## The substitute: the shipped asset, decoded independently

For any screen whose content comes from a resource file, the original's frame at that moment is determined by
that file: the game blits the card, it does not repaint it. So the reference for a still screen is the asset
itself, decoded by code that is *not* the port's loader.

Two independent pieces make the comparison strong:

1. **Reference side** — `scripts/playtest_compare_slides.py::pcx_indices` decodes a shipped `.pcx` from its
   own header (RLE after the 128-byte header, a byte with bits 6-7 set is a fill of `byte & 0x3f` copies,
   destination size from the header bounding box). Written from the format, not from the port.
2. **Port side** — `NIGHT_VIDRAW=1` makes the host dump the **screen arena bytes** behind each screenshot
   (`g_video_ram`, the model the game's own loaders write and the presenter reads) plus the DAC table it used.

Comparing (1) to (2) is a comparison of the game's screen model to the shipped asset, with no dependence on
the palette ramp in progress, on BMP row order, or on the host's presentation. This is the same method used
earlier for the intro film, where the reference was an ffmpeg decode of `opening.fli` at matched frame
indices (docs: intro-artifact closure, L1 1.45-2.00 per frame against the ffmpeg reference).

Measured so far with it:

| screen | arena vs asset decode |
|---|---|
| `logo.pcx` at the logo moment (frame 1 of the run) | **307200/307200 bytes identical** |
| every intro slide of episodes 1, 2, 3, 6, 8, 9 (docs/62) | **307200/307200 each** |
| `stage1.pcx` while the card holds (frame 2463) | **307200/307200** |
| `title1.pcx` once the title settles (frame 2298+) | 289838/307200 -- the title screen is *not* a bare card: the game composites over it (see below) |

## What the substitute cannot prove

- **Pacing against the original's clock.** A still frame carries no timing. Title-animation pacing is instead
  pinned by the emulated-clock model (PIT 490 Hz tick model, the retrace-wait fix in docs/47) and by counting
  the waits the shipped code asks for; a measured port/original frame-content-at-tick comparison is not
  obtainable here, and that is stated rather than papered over.
- **Anything the original draws procedurally** (room tiles, sprite blits, fades in flight). For those the
  evidence stays structural: per-entity asset-resolution assertions, per-frame edge-noise measures, and dense
  frame series.

## Consequence for the task contracts

Contracts whose wording requires the original's frame ("compared with the original's logo frame", "match the
original frames at the same .fli frame index") have their comparison half blocked by the environment. Each is
annotated in the playtest log with what *was* measured and what remains unmeasurable here, so nothing reads as
verified when it is not.

## Logo frame: the same byte-exact basis applies

`route_attract_wait.txt` with `NIGHT_VIDRAW=1` gives an arena dump behind the logo shot. Decoding
`logo.pcx` (640x480, 307200 B) straight out of `NIGHT.RES` with the independent decoder in
`scripts/playtest_compare_slides.py::pcx_indices`:

| shot | arena bytes equal to the shipped logo.pcx decode |
|---|---|
| `0000_card_00.raw` (just before the announce) | 307200 / 307200 |
| `0001_logo.pcx.raw` (the logo shot itself) | 307200 / 307200 |
| `0003_card_01.raw` (200 ms later, still on the logo) | 307200 / 307200 |

So the company logo is not merely "visible": the bytes the port puts in the screen arena are byte-identical to
the shipped card, and the `card_NN` series shows it held across the window-slide presentation (docs/51). The
decode path itself is separately pinned by `make -C port selftest` (`[pcx] logo.pcx`, docs/44).

## Nested screens on the same basis: help, level-up, game-over

`NIGHT_VIDRAW=1` arena dumps plus the independent PCX decoder give a byte-level verdict for the nested
screens reached through the `NIGHT_SHOWSCREEN` warp (docs/61). Cards are 320x200 here, so the comparison
region is the first 64000 arena bytes.

| screen | measured against the shipped card |
|---|---|
| help (`NIGHT_SHOWSCREEN=help`) | `help1.pcx` **0 / 64000 bytes differ** (byte-identical, two frames) |
| level-up / status (`=levelup`) | `levelup2.pcx` **677-881 / 64000 differ (1.06-1.38%)**, confined to rows 25..178 x cols 24..134 -- the value/number column the screen draws from `number1.spr` / `number2.spr` / `levelup.spr`. `levelup2.pcx` is consistently the closer card than `levelup.pcx`, exactly as the loader trace predicted (`fn_036190: levelup2.pcx`, docs/61) |
| game-over / continue (`=gameover`) | one frame is **byte-identical to `continue.pcx`** (0/64000); frames showing the GAME OVER art match each of the three shipped layers (`gameove1/2/3.pcx`) at 94.3-95.1%, and **95.1% of all pixels match at least one layer** -- i.e. the screen is a shipped card plus the `over.spr` character art and the CREDIT digits (~5%), which is how `fn_038445` composites it |

Read the level-up row this way: the card is present and correct to within the pixels the screen is *supposed*
to overwrite. What this basis still cannot say is pacing and the intermediate frames -- that half stays with
docs/46 / docs/64.

## Entity sprites in gameplay, on the same basis

The entity draw (`fn_012efa`, the routine every entity sprite goes through) now offers `NIGHT_ENTSPR=1`, which
dumps each composited entity frame's row-RLE stream and the bytes the blit wrote (bounded to 120 cells per run,
skipping cells whose screen position is negative because the blit clamps those). `scripts/playtest_verify_spr.py`
decodes the stream independently:

```
  entspr   compared   48301  matched   48301  mismatched     0
```

over the stage-1 route: 120 entity-frame composites, 48,301 covered bytes, no mismatch. Which asset each entity
uses is pinned separately -- the 101 `.spr` / 101 `.act` per-type name arrays hash-equal the copies read out of
`NIGHT.EXE` (DS `0x51d88` / `0x52198`), and stage 1's two enemies are type 31 -> `ena-1.spr` / `ena-1.act`.
Together those two facts replace the old "enemies draw the main character" symptom with a byte-level claim.

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

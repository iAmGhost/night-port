#!/usr/bin/env python3
"""Make the browser port's install icons out of the original game's own icon.  docs/457.

`extracted/NIGHT.ICO` is a single 32x32 RGBA entry: the protagonist's face, 256 colours, the same art the
DOS shell showed. The port's home-screen icon uses that art rather than a redrawn mark, and every output
size is a whole multiple of 32 so the upscale stays exact -- NEAREST at 6x/12x/16x repeats pixels evenly,
which is the only kind of enlargement a 32-pixel sprite takes without smearing. A 512 icon resampled with
LANCZOS is a blur; a 512 icon at 16x is the same picture, just bigger, which is also what the game looks
like on the screen next to it.

Sizes: `icon-192` and `icon-512` are full-bleed (the manifest's "any" purpose); `icon-maskable-512` puts
the same sprite at 12x inside a #0b0b0b ground, because a maskable icon gets its outer ring cut away by the
launcher and the face would lose its hair.

Run it with uv:  uv run --with pillow scripts/gen/gen_app_icons.py
"""
from __future__ import annotations

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:                                     # pragma: no cover - the one dependency of this file
    sys.exit("needs Pillow:  uv run --with pillow scripts/gen/gen_app_icons.py")

REPO = Path(__file__).resolve().parents[2]
SRC = REPO / "extracted" / "NIGHT.ICO"
OUT = REPO / "wasm" / "runtime"
GROUND = (11, 11, 11, 255)


def sprite() -> Image.Image:
    im = Image.open(SRC).convert("RGBA")
    im.load()
    if im.size != (32, 32):
        sys.exit(f"{SRC.name} is {im.size}, not the 32x32 this script assumes")
    return im


def tile(w: int, scale: int, full_bleed: bool) -> Image.Image:
    """The sprite scaled by a whole number, either edge to edge or set into a dark square."""
    art = sprite().resize((32 * scale, 32 * scale), Image.NEAREST)
    if full_bleed and art.size == (w, w):
        return art
    out = Image.new("RGBA", (w, w), GROUND)
    out.paste(art, ((w - art.width) // 2, (w - art.height) // 2), art)
    return out


def main() -> int:
    if not SRC.is_file():
        sys.exit(f"{SRC} missing -- run `make extract` from the repo root first")
    OUT.mkdir(parents=True, exist_ok=True)
    # (name, canvas, whole-number scale, full bleed). 192=6x, 512=16x, maskable 512=12x set in a ground.
    for name, size, scale, bleed in [("icon-192.png", 192, 6, True),
                                     ("icon-512.png", 512, 16, True),
                                     ("icon-maskable-512.png", 512, 12, False)]:
        if size % (32 * scale) and bleed:
            sys.exit(f"{name}: {size} is not a whole multiple of {32 * scale}")
        img = tile(size, scale, bleed)
        img.save(OUT / name, optimize=True)
        print(f"wrote {OUT / name}  {size}x{size} from {SRC.name} at {scale}x")
    return 0


if __name__ == "__main__":
    sys.exit(main())

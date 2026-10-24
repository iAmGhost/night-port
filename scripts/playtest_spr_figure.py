#!/usr/bin/env python3
"""Figure: the stage-1 player and an enemy as the port wrote them, and the same art in the original's frames.

The port (NIGHT_ENTSPR=1) dumps, for every entity composite, the bytes the blitter actually wrote at the
composite's rectangle (`menupix` rows, palette indices) together with the entity's type and the shipped .spr/.act
that type names. Those rows are the composite exactly as the game composed it -- frame-accurate, no dependence on
when a screenshot was sampled -- so they are mapped through the stage-1 room palette (scratch/res/stage1.wal +
0x4e21e, the 768 bytes the game itself pushes; docs/70) and used as templates to find the same art inside the
original's captured stage-1 frames. Both channel orders are tried because the mode-13h capture payload is BGRX
(docs/70); the residuals are printed, so the figure is a measurement.

Companion to scripts/playtest_check_entity_sprites.py, which makes the byte-level claims (docs/73).
"""
from __future__ import annotations

import os
import re
import sys

import numpy as np
from PIL import Image, ImageDraw
from numpy.lib.stride_tricks import sliding_window_view

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from playtest_check_entity_sprites import cell_re, run_port  # noqa: E402

ORIG = "/tmp/dbgauto/orig_full"
OUT = os.path.join(ROOT, "docs", "manual-playtest", "07-shots", "sprites_player_enemy_vs_original.png")
SCALE, SIDE = 3, 24


def room_palette_rgb() -> np.ndarray:
    raw = open(os.path.join(ROOT, "scratch", "res", "stage1.wal"), "rb").read()[0x4e21e:0x4e21e + 768]
    return np.array([min(255, round(c * 255 / 63)) for c in raw], dtype=np.uint8).reshape(256, 3)


def best_match(ref: np.ndarray, tmpl: np.ndarray) -> tuple[float, int, int, float]:
    """(best meanSqErr, x, y, mean meanSqErr over every placement) -- the last figure is what a match has to beat."""
    h, w = tmpl.shape[:2]
    sw = sliding_window_view(ref, (h, w, ref.shape[2]))
    d = sw.reshape(sw.shape[0], sw.shape[1], -1) - tmpl.reshape(-1).astype(np.int16)
    ssd = (d * d).mean(axis=2)
    oy, ox = np.unravel_index(int(np.argmin(ssd)), ssd.shape)
    return (float(ssd[oy, ox]), int(ox), int(oy), float(ssd.mean()))


def main() -> int:
    log, shots = run_port()
    lines = open(log, "r", errors="replace").read().splitlines()
    pix_re = re.compile(r"menupix k=(\d+) r=(\d+) ([0-9a-f]+)")

    patches: dict[str, tuple] = {}
    cur = None
    for l in lines:
        m = cell_re.search(l)
        if m:
            g = m.groups()
            cur = {"W": int(g[3]), "H": int(g[4]), "typ": g[5], "spr": g[6], "k": int(g[0]),
                   "mode": int(g[8]), "rows": {}}
            continue
        m = pix_re.search(l)
        if m and cur is not None:
            cur["rows"][int(m.group(2))] = bytes.fromhex(m.group(3))
            if len(cur["rows"]) >= cur["H"]:
                kind = "player" if cur["typ"] == "0" else "enemy"
                patches[kind] = cur          # keep the last complete composite of each kind
                cur = None

    if len(patches) != 2:
        print(f"FAIL: got composites for {sorted(patches) or 'nothing'}; need both player (type 0) and an enemy")
        return 1
    pal = room_palette_rgb()

    man = [l.split("\t") for l in open(os.path.join(shots, "manifest.tsv")).read().splitlines()[1:]]
    play = [f for f in man if f[3] == "playing"] or [f for f in man if int(f[7]) == 320]
    port_path = os.path.join(shots, "%04d_%s.bmp" % (int(play[-1][0]), play[-1][3]))
    port = np.asarray(Image.open(port_path).convert("RGB"))

    origs = sorted(os.path.join(ORIG, n) for n in os.listdir(ORIG) if "gameplay" in n)
    # The mode-13h capture payloads are BGRX (docs/70), so the stored PNGs are channel-mirrored: load them swapped
    # (SWAP=False falls back to the raw file for VESA-era captures).
    SWAP = os.environ.get("SPR_FIG_SWAP", "1") == "1"
    frames = [(n, (np.asarray(Image.open(n).convert("RGB"))[:, :, ::-1] if SWAP else
                   np.asarray(Image.open(n).convert("RGB"))).astype(np.int16)) for n in origs]
    print(f"port shot {os.path.basename(port_path)} {port.shape[1]}x{port.shape[0]}; "
          f"original stage-1 frames {len(frames)}")

    results = {}
    for kind in ("player", "enemy"):
        c = patches[kind]
        idx = np.zeros((c["H"], c["W"]), dtype=np.uint8)
        for r, b in c["rows"].items():
            idx[r, :len(b)] = np.frombuffer(b, dtype=np.uint8)[:c["W"]]
        patch = pal[idx]
        tmpl = patch[(c["H"] - SIDE) // 2:(c["H"] - SIDE) // 2 + SIDE,
                     (c["W"] - SIDE) // 2:(c["W"] - SIDE) // 2 + SIDE]
        best = second = None
        for n, fr in frames:
            for order in (0, 1):
                ref = fr[:, :, ::-1] if order else fr
                ssd, ox, oy, avg = best_match(ref, tmpl)
                if best is None or ssd < best[0]:
                    best = (ssd, n, ox, oy, order, avg)
                if order:
                    second = ssd if second is None else min(second, ssd)
        ssd, n, ox, oy, order, avg = best
        ofull = np.asarray(Image.open(n).convert("RGB"))
        ocrop = (ofull[:, :, ::-1] if SWAP else ofull)[oy:oy + c["H"], ox:ox + c["W"]]
        print(f"{kind}: {c['spr']} cell k={c['k']} {c['W']}x{c['H']} mode={'mirrored' if c['mode'] else 'straight'}"
              f" -> original match {os.path.basename(n)} at ({ox},{oy}) channels="
              f"{'RGB' if order == 0 else 'BGR'} meanSqErr={ssd:.1f} vs {avg:.1f} averaged over every placement "
              f"of that {SIDE}x{SIDE} core (other order best {second:.1f}; perfect match 0, worst {255.0**2:.0f})")
        results[kind] = (patch, ocrop, ssd, os.path.basename(n))

    bh, bw = port.shape[:2]
    gap, lab = 12, 16
    ch = bh + lab + sum(patches[k]["H"] * SCALE + lab + 8 for k in ("player", "enemy"))
    canvas = Image.new("RGB", (bw * 2 + gap, ch), (24, 24, 28))
    dr = ImageDraw.Draw(canvas)
    canvas.paste(Image.open(port_path).convert("RGB"), (0, 0))
    mid = frames[len(frames) // 2][0]
    canvas.paste(Image.open(mid).convert("RGB"), (bw + gap, 0))
    dr.text((2, bh + 2), "port stage 1 (screenshot)", fill=(235, 235, 235))
    dr.text((bw + gap + 2, bh + 2),
            f"original, episode 1 gameplay ({os.path.basename(mid)}; mode-13h capture un-mirrored)",
            fill=(235, 235, 235))
    row = bh + lab + 6
    for kind in ("player", "enemy"):
        patch, ocrop, ssd, oname = results[kind]
        c = patches[kind]
        canvas.paste(Image.fromarray(patch).resize((c["W"] * SCALE, c["H"] * SCALE), Image.NEAREST), (0, row))
        canvas.paste(Image.fromarray(ocrop).resize((c["W"] * SCALE, c["H"] * SCALE), Image.NEAREST), (bw + gap, row))
        dr.text((c["W"] * SCALE + 10, row + 6),
                f"{kind}: {c['spr']} cell k={c['k']} {c['W']}x{c['H']}, as the port's blitter wrote it",
                fill=(255, 220, 120))
        dr.text((bw + gap + c["W"] * SCALE + 10, row + 6),
                f"same art at the same size in {oname}, meanSqErr {ssd:.1f}", fill=(150, 235, 150))
        row += c["H"] * SCALE + lab + 8
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    canvas.save(OUT)
    print(f"wrote {OUT}  ({canvas.size[0]}x{canvas.size[1]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())

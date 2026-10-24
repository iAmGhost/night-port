#!/usr/bin/env python3
"""Compare the port's slide-card screenshots against the shipped PCX cards.

This is the check the slideshows task contract asks for: "a headless run shows the slide sequence
(screenshot per slide, matching the corresponding PCX decode) before gameplay starts".  It does not need
the original game running -- the card is the ground truth the original would have blitted -- so unlike most
of the playtest comparisons it is not blocked on the DOSBox capture path (docs/46).

The decoder mirrors the game's own still-image loader (port/src/f/f.c fn_036190, Ghidra 0x0003621b-0x00036285):
data starts after the 128-byte header, and the stream is the 6-bit-count PCX RLE -- a byte with bits 6-7 set
is a FILL of (byte & 0x3f) copies of the following byte, anything else is a literal -- read until 0xfa00
(64000 = 320x200) destination bytes exist.  The destination bytes are palette *indices*, and the port's
screenshots are written as 8-bit indexed bitmaps, so index arrays are compared directly.

Usage: playtest_compare_slides.py <shot-dir> [stage ...]
"""
import glob
import os
import struct
import sys

RES = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "scratch", "res")


def pcx_indices(path):
    """Decode the RLE stream.  The destination size is the header bounding box, not a constant: the
    320x200 cards go through fn_036190 (0xfa00 bytes, Ghidra 0x0003621b) while the slide cards are
    640x480 and go through fn_03628e (0x4b000 bytes, 0x00036355) with the window slid per 64 KB chunk."""
    d = open(path, "rb").read()
    if d[0] != 0x0A:
        raise ValueError("not a PCX: %s" % path)
    x0, y0, x1, y1 = struct.unpack("<HHHH", d[4:12])
    want = (x1 + 1) * (y1 + 1)
    i, out = 128, bytearray()
    while len(out) < want:
        b = d[i]
        i += 1
        if b & 0xC0 == 0xC0:
            out += bytes([d[i]]) * (b & 0x3F)
            i += 1
        else:
            out.append(b)
    return bytes(out)


def bmp_rgb(path):
    """The host writes the view as 32-bit RGBA8888 (SDL masks R=0xff000000 G=0x00ff0000 B=0x0000ff00,
    i.e. R is the first byte in memory), so read it as RGB triples."""
    d = open(path, "rb").read()
    off = struct.unpack("<I", d[10:14])[0]
    h = struct.unpack("<i", d[22:26])[0]
    w = struct.unpack("<I", d[18:22])[0]
    bpp = struct.unpack("<H", d[28:30])[0]
    if bpp != 32:
        raise ValueError("expected 32-bit BMP, got %d bpp: %s" % (bpp, path))
    rows = h if h > 0 else -h
    stride = ((w * bpp + 31) // 32) * 4
    rowbytes = w * bpp // 8                     # bytes per row, not pixels times bits
    # A BMP with a positive height is bottom-up, so file row 0 is the image's last row; the game's arena is
    # top-down.  Reading the file order silently flips the picture.
    order = range(rows - 1, -1, -1) if h > 0 else range(rows)
    px = b"".join(d[off + y * stride:off + y * stride + rowbytes] for y in order)
    # The host builds an RGBA8888 surface and SDL_SaveBMP writes it in the BMP's byte order, which is
    # B,G,R,(pad) -- reading the first three bytes as R,G,B silently swaps the channels.
    return w, rows, [px[i + 2: i + 3] + px[i + 1: i + 2] + px[i:i + 1] for i in range(0, len(px), 4)]


def pcx_rgb(path):
    """Card as RGB triples through its own trailer palette (PCX marker 0x0C + 768 bytes).  The game loads
    that table and shifts it right by 2 for the 6-bit DAC (fn_036190 at 0x000361f1-0x00036203), so the
    quantized copy is what a faithful screen can actually show."""
    d = open(path, "rb").read()
    idx = pcx_indices(path)
    pal = d[-768:]
    assert d[-769] == 0x0C, "no PCX trailer palette in %s" % path
    return [pal[3 * c:3 * c + 3] for c in idx], [(pal[3 * c] >> 2) << 3 for c in idx], \
           [(pal[3 * c + 1] >> 2) << 3 for c in idx], [(pal[3 * c + 2] >> 2) << 3 for c in idx]


def pcx_size(path):
    d = open(path, "rb").read()
    x0, y0, x1, y1 = struct.unpack("<HHHH", d[4:12])
    return x1 + 1, y1 + 1


def l1(a, b):
    """Mean per-pixel L1 over the three channels; a and b are lists of 3-byte tuples."""
    n = min(len(a), len(b))
    return sum(abs(a[i][c] - b[i][c]) for i in range(n) for c in range(3)) / (n * 3)


def fit_l1(shot, card):
    """Structural match: fit a per-channel affine map (gain, offset) from the screenshot to the card and
    report the residual.  A slide is sampled while the game's fade-in is still ramping the DAC (the loader
    hands the palette to fn_040660 with 0x19 steps, port/src/f/f.c 0x0003b0ec), so absolute colour cannot
    match the card's trailer palette at every sample -- but the *image* must, and the fitted gain is itself
    a measurement of the fade.  Returns (residual L1 per channel, mean gain)."""
    n = min(len(shot), len(card))
    res, gains = 0.0, []
    for c in range(3):
        xs = [shot[i][c] for i in range(n)]
        ys = [card[i][c] for i in range(n)]
        mx, my = sum(xs) / n, sum(ys) / n
        sxx = sum((x - mx) ** 2 for x in xs)
        sxy = sum((xs[i] - mx) * (ys[i] - my) for i in range(n))
        a = (sxy / sxx) if sxx else 0.0
        b = my - a * mx
        gains.append(a)
        res += sum(abs(a * xs[i] + b - ys[i]) for i in range(n)) / n
    return res / 3.0, sum(gains) / len(gains)


def l1_q(shot, qr, qg, qb):
    n = min(len(shot), len(qr))
    tot = 0
    for i in range(n):
        tot += abs(shot[i][0] - qr[i]) + abs(shot[i][1] - qg[i]) + abs(shot[i][2] - qb[i])
    return tot / (n * 3)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    shots = sys.argv[1]
    stages = [int(s) for s in sys.argv[2:]] or list(range(1, 10))
    bad = 0
    for stage in stages:
        cards = sorted(glob.glob(os.path.join(RES, "%ds*.pcx" % stage)) +
                       glob.glob(os.path.join(RES, "%de*.pcx" % stage)))
        files = sorted(glob.glob(os.path.join(shots, "*.bmp")))
        # manifest.tsv columns: seq t_ms frames event resource stage entities W H
        man = []
        mpath = os.path.join(shots, "manifest.tsv")
        if os.path.exists(mpath):
            for line in open(mpath).read().splitlines()[1:]:
                f = line.split("\t")
                if len(f) >= 9:
                    man.append({"seq": f[0], "frames": int(f[2]), "event": f[3], "res": f[4]})
        rows = []
        for c in cards:
            name = os.path.basename(c)[:-4]
            # the harness names a shot after the resource it saw: NNNN_<name>.pcx.bmp, and the script's own
            # slide events are NNNN_slide_<name>.bmp -- accept either.
            # The card is announced by its loader but only *displayed* afterwards, so the shot taken at the
            # announce cannot match the card.  The periodic `card_NN` series (200 ms while the view is
            # 640x480) covers the display window: take every such frame whose last-seen resource is this
            # card and keep the closest match.
            cand = [os.path.join(shots, "%04d_%s.bmp" % (int(m["seq"]), m["event"]))
                    for m in man
                    if m["res"] in (name, name + ".pcx") and m["event"].startswith("card_")]
            cand = [f for f in cand if os.path.exists(f)]
            if not cand:
                tag = ("ending card, not reached by this route"
                       if name[len(str(stage)):].startswith("e")
                       else "no card-mode frame while this resource was current")
                rows.append((name, tag, None))
                continue
            # Preferred, and the honest one: the host can dump the *screen arena bytes* behind a shot
            # (NIGHT_VIDRAW=1, port/src/app/main.c shot_named).  Those bytes are the game's own screen model,
            # so comparing them with an independent decode of the shipped card proves the card is on screen
            # without any dependence on how the palette happened to be ramping at the sample instant, or on
            # how a BMP encoder lays out rows and channels.
            arena_hit = None
            for f in cand:
                rp = f[:-4] + ".raw"
                if not os.path.exists(rp):
                    continue
                raw = open(rp, "rb").read()
                arena = raw[:pcx_size(os.path.join(RES, name + ".pcx"))[0] *
                            pcx_size(os.path.join(RES, name + ".pcx"))[1]]
                card_idx = pcx_indices(os.path.join(RES, name + ".pcx"))
                same = sum(1 for i in range(min(len(arena), len(card_idx))) if arena[i] == card_idx[i])
                frac = 100.0 * same / len(card_idx)
                if arena_hit is None or frac > arena_hit[0]:
                    arena_hit = (frac, os.path.basename(rp), same, len(card_idx))
            if arena_hit is not None:
                frac, rp, same, tot = arena_hit
                rows.append((name, "%s arena %d/%d bytes" % (rp, same, tot), 100.0 - frac))
                continue

            card, qr, qg, qb = pcx_rgb(os.path.join(RES, name + ".pcx"))
            best, bestf, bestgain, bestraw = None, None, None, None
            for f in cand:
                w, h, px = bmp_rgb(f)
                if (w, h) != pcx_size(os.path.join(RES, name + ".pcx")):
                    continue
                sc, gain = fit_l1(px, card)
                if best is None or sc < best:
                    best, bestf, bestgain, bestraw = sc, os.path.basename(f), gain, l1(px, card)
            if best is None:
                rows.append((name, "no frame at the card's own resolution", None))
            else:
                rows.append((name, "%s (fade gain %.2f, raw L1 %.1f)" % (bestf, bestgain, bestraw), best))
        play = [f for f in files if os.path.basename(f).endswith("playing.bmp")]
        print("stage %d: %d shipped card(s)%s" % (
            stage, len(cards),
            ", %s" % os.path.basename(play[0]) if play else ", no `playing` shot"))
        for name, size, score in rows:
            if score is None:
                # An intro slide with no captured frame is a real failure (the contract is that each episode's
                # slide sequence is *shown*); an ending card the route never reaches is not.
                intro = not name[len(str(stage)):].startswith("e")
                print("    %-10s %s%s" % (name, size if isinstance(size, str) else "NO SHOT CAPTURED",
                                          "" if not intro else "   <-- FAIL"))
                if intro:
                    bad += 1
            else:
                # score is in "percent of card bytes NOT matched" when an arena dump exists (so < 0.1 is the
                # pass bar), or the fade-fitted RGB residual otherwise (a mismatched image sits far above 12).
                ok = score < 0.1 if "arena" in str(size) else score < 12.0
                print("    %-10s %-34s score=%6.2f  (%s)" % (
                    name, size, score, "MATCH" if ok else "differs"))
                if not ok:
                    bad += 1
    print("cards checked, problems:", bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

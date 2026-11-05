#!/usr/bin/env python3
"""docs/373 -- the CG-show and ending cards, compared the docs/62 way.

The nine-episode sweep (docs/363) proves the port holds its STAGE cards byte-exactly, but the `na<stage>NNNN.pcx`
picture shows and the ending cards were never compared at all: docs/62 lists slide cards, and the warp route walks
through the shows without playing them (docs/363 §4).  The SDL host's keyed route DOES reach them
(scripts/playtest/route_show_keyed.txt, docs/59), and its shot dump writes the arena bytes under the name of the
resource the loader just put on screen (src/app/main.c:1147), so the comparison is index-vs-index exactly like
docs/62's -- no palette, no DAC fade, no host scaling.

    ./build/night --script scripts/playtest/route_show_keyed.txt --warp-stage 8 \
        --shots /tmp/na8 --no-audio --frames 9000
    python3 scripts/pocket/show_cards.py /tmp/na8
"""
import os
import sys
import pathlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sweep_report import pcx_indices, identical, RES      # the same shipped-PCX decode, the same criterion


def main(shots_dir: str) -> int:
    man = pathlib.Path(shots_dir) / "manifest.tsv"
    if not man.exists():
        print(f"no manifest in {shots_dir} -- run the keyed route with --shots {shots_dir}")
        return 1
    rows = man.read_text().splitlines()[1:]
    best: dict[str, tuple[int, int, int, str]] = {}
    for r in rows:
        c = r.split("\t")
        if len(c) < 9:
            continue
        seq, frames, res, w, h = c[0], c[2], c[4], int(c[7]), int(c[8])
        if not res.endswith(".pcx"):
            continue
        # The manifest prints seq as a number ("205") while the shot files are zero-padded ("0205_na80001.pcx.raw"),
        # so the name is rebuilt from the padded form -- the first version of this script found no files at all.
        # Shot files are named after the label that triggered them -- the resource name at a load edge, the script's
        # own label (pre_show, show_scan) for a keyed shot -- so the exact name is tried first and a sequence-number
        # glob is the fallback.  Without the fallback a script-labelled shot is invisible to the comparison.
        raw = pathlib.Path(shots_dir) / f"{int(seq):04d}_{res}.raw"
        if not raw.exists():
            hits = sorted(pathlib.Path(shots_dir).glob(f"{int(seq):04d}_*.raw"))
            raw = hits[0] if hits else raw
        card = os.path.join(RES, res)
        if not raw.exists() or not os.path.exists(card):
            continue
        cw, ch, idx = pcx_indices(card)
        if idx is None:
            continue
        data = raw.read_bytes()[:w * h]
        if len(data) != len(idx):
            # A card smaller than the screen model is compared at its natural offset, and the offset is searched
            # rather than assumed: 0, then every row boundary of the shot.
            n, best_off, best_same = len(idx), 0, 0
            for off in range(0, len(data) - n + 1, max(w, 1)):
                same = identical(data[off:off + n], idx)
                if same > best_same:
                    best_same, best_off = same, off
            same, need = best_same, n
            note = f" offset {best_off}"
        else:
            same, need, note = identical(data, idx), len(idx), ""
        # docs/373's finding: the picture-show loader (fn_03a70e and its PCX path) treats index 255 as a SKIP key --
        # the card's 255 pixels are the only ones the port does not write, and every non-255 pixel is exact.  So a
        # show card is judged two ways: the plain docs/62 criterion, and the same comparison with the card's 255
        # pixels excluded.  Reporting only the first calls a correct render "partial".
        key = 255
        keep = [i for i, v in enumerate(idx) if v != key]
        same_key = sum(1 for i in keep if data[i] == idx[i]) if len(data) == len(idx) else 0
        prev = best.get(res)
        rec = (same, need, int(frames), f"{seq}_{res}.raw{note}", same_key, len(keep))
        if prev is None or rec[4] > prev[4] or (rec[4] == prev[4] and same > prev[0]):
            best[res] = rec

    exact = exact_key = 0
    for res, (same, need, frames, path, same_key, keep_n) in sorted(best.items(),
                                                                     key=lambda kv: -kv[1][4] / max(kv[1][5], 1)):
        ratio = same / max(need, 1)
        rkey = same_key / max(keep_n, 1)
        flag = "IDENTICAL" if same == need else ("EXACT-OFF-255" if rkey == 1.0 else
                 ("mostly" if rkey >= 0.9 else ("partial" if rkey >= 0.5 else "no")))
        if same == need:
            exact += 1
        if rkey == 1.0:
            exact_key += 1
        print(f"  {res:<16} plain {same:>7}/{need} ({ratio * 100:6.2f}%)   non-255 {same_key:>7}/{keep_n} "
              f"({rkey * 100:6.2f}%)  frame {frames:>6}  {path:<28} {flag}")
    print(f"== {exact}/{len(best)} cards byte-identical outright; {exact_key}/{len(best)} exact on every pixel the "
          f"card does not encode as index 255 ==")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "/tmp/na8"))

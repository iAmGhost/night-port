"""Compare the port's .fli playback frames against an independent decode of the same .fli.

The reference frames come from ffmpeg's Autodesk flic decoder applied to the shipped
scratch/res/opening.fli, so this cross-check needs neither DOSBox nor the port's own decoder.
Both sides are reduced to 40x25 luma, and each port shot is matched to its nearest reference frame.
"""
import sys, glob, os
import numpy as np
from PIL import Image

def load(p):
    im = Image.open(p).convert('L').resize((80, 50), Image.NEAREST)
    return np.asarray(im, dtype=np.float32)

ref_dir, shot_glob = sys.argv[1], sys.argv[2]
refs = np.stack([load(p) for p in sorted(glob.glob(os.path.join(ref_dir, '*.png')))])
refs = refs.reshape(len(refs), -1)
print(f"reference frames: {refs.shape[0]}")

for shot in sorted(glob.glob(shot_glob)):
    q = load(shot).reshape(1, -1)
    d = np.abs(refs - q).mean(axis=1)
    i = int(d.argmin())
    best, second = d[i], np.sort(d)[max(1, int((d < 6).sum()))]
    # per-region worst error to localise artifacts
    err = np.abs(refs[i].reshape(50, 80) - q.reshape(50, 80))
    rows = err.max(axis=1)
    worst = int(rows.argmax())
    print(f"{os.path.basename(shot):28s} best ref #{i:4d}  L1={d[i]:6.2f}  "
          f"worst row {worst:2d} (err {rows[worst]:5.1f})  5th-worst rows {sorted(np.argsort(rows)[-5:])}")

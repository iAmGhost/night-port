#!/bin/sh
# docs/120 §14: per-episode coverage curve on the budgeted build (docs/146 §9).
# One fuzz run per episode, fixed shape, hard caps; nothing unbounded.
OUT=/tmp/rig/fz18
rm -rf "$OUT"; mkdir -p "$OUT"
for st in 1 2 3 4 5 6 7 8 9; do
  echo "== stage $st"
  timeout 1500 python3 scripts/rig/fuzz.py --episodes "$st" --seeds 4 --workers 4 --cap 110 \
      --frames 6600 --rounds 2 --events 80 --inject --families 15 --out "$OUT/st$st" \
      > "$OUT/st$st.out" 2>&1
  grep -E "^final coverage" "$OUT/st$st.out" | sed "s/^/stage $st: /"
done
python3 - <<'PY'
import os, glob
names = set()
rows = []
# universe = every shipped-named function the census keys coverage to (503 per docs/120)
for cov in sorted(glob.glob('/tmp/rig/fz18/st*/coverage.txt')):
    n_ent = 0
    for line in open(cov):
        p = line.split()
        if p and p[0].startswith('fn_') or p and p[0].startswith(('case','cb_','lab','sub','win','main','dispatch')):
            n_ent += 1
            names.add(p[0])
    rows.append((cov.split('/')[4], n_ent))
for tag, n in rows:
    print(f"{tag}: entered {n}")
print(f"UNION across the nine episodes: {len(names)} function names entered")
PY

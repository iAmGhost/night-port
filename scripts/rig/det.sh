#!/usr/bin/env bash
# Determinism experiment: same pinned-cycles boot + same input stream, twice, from fresh containers.
# Usage: scripts/rig/det.sh [runs] [seconds]
set -uo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
RUNS=${1:-2}
DUR=${2:-45}
DRIVE=${DRIVE:-$HERE/drives/title_basic.txt}
OUT=${OUT:-/tmp/rig/det}
rm -rf "$OUT"; mkdir -p "$OUT"

for i in $(seq 1 "$RUNS"); do
  echo "== run $i: fresh container"
  RUN_GAME=1 bash "$HERE/bringup.sh" > "$OUT/bringup_$i.log" 2>&1
  sleep 12                                   # let NIGHT load before calibration probes
  timeout 120 python3 "$HERE/calibrate_ds.py" > "$OUT/calib_$i.log" 2>&1 || { echo "calib failed"; tail -5 "$OUT/calib_$i.log"; exit 1; }
  timeout $((DUR + 120)) python3 "$HERE/replay_original.py" --drive "$DRIVE" --out "$OUT/run$i" --seconds "$DUR" --hz 4 | tail -1
  timeout 20 curl -s --noproxy '*' -X POST -H "Authorization: Bearer $(cat /tmp/rig/.token)" \
    http://localhost:8386/api/v1/control/shutdown >/dev/null 2>&1 || true
done
python3 "$HERE/det_report.py" "$OUT"/run* || true
python3 "$HERE/entropy_report.py" "$OUT"/run*

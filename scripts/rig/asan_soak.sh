#!/usr/bin/env bash
# Nine-episode ASan/UBSan soak (docs/126 §6, closeout contract: 0 reports / 0 SEGV / all rc=0).
#   OUT=/tmp/rig/soak JOBS=4 CAP=300 FRAMES=4000 scripts/rig/asan_soak.sh
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${OUT:-/tmp/rig/soak}
JOBS=${JOBS:-4}
CAP=${CAP:-300}
FRAMES=${FRAMES:-4000}
BIN=${BIN:-./port/test/night-asan}
BASE=${BASE:-scripts/rig/drives/title_basic.txt}
mkdir -p "$OUT"
cd "$ROOT" || exit 1
for s in 1 2 3 4 5 6 7 8 9; do
  (
    ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 UBSAN_OPTIONS=print_stacktrace=0 \
    SDL_AUDIODRIVER=dummy \
    timeout "$CAP" $BIN --res-dir ./NIGHT --script "$BASE" --warp-stage "$s" --frames "$FRAMES" \
      > "$OUT/stage$s.log" 2>&1
    rc=$?
    err=$(grep -c "runtime error" "$OUT/stage$s.log")
    asan=$(grep -c "ERROR: AddressSanitizer" "$OUT/stage$s.log")
    seg=$(grep -cE "SEGV on unknown address|Segmentation fault" "$OUT/stage$s.log")
    printf 'stage %s rc=%-3s ubsan=%-4s asan=%-3s segv=%-3s %s\n' "$s" "$rc" "$err" "$asan" "$seg" \
      "$(grep -o 'frames presented: [0-9]*' "$OUT/stage$s.log" | tail -1)"
  ) &
  # bash 3.2 (macOS default) has no `wait -n`: poll instead of blocking on one pid
  while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 2; done
done
wait
echo "soak done -> $OUT"

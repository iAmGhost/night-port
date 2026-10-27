#!/usr/bin/env bash
# One command, one route, both sides, one divergence report (rig-runtime contract).
#
#   scripts/rig/route.sh [drive] [seconds]
#
# The original runs twice in fresh containers (state replay, then a screen capture) because it must be
# started from a clean boot for the anchor to mean the same thing; the port runs once, in the host.
# Every step is timeout-capped per AGENTS.md, and the report carries timestamps.
set -uo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
DRIVE=${1:-$HERE/drives/title_basic.txt}
DUR=${2:-30}
OUT=${OUT:-/tmp/rig/route-$(date +%Y%m%d-%H%M%S)}
PORT_ENV=${PORT_ENV:-}
mkdir -p "$OUT"
say() { printf '%s  %s\n' "$(date -u +%H:%M:%S)" "$*"; }

say "route: drive=$DRIVE duration=${DUR}s out=$OUT"

# ---- port side -------------------------------------------------------------------------
say "port: night --rig-dump"
( cd "$ROOT" && env $PORT_ENV timeout $((DUR + 40)) ./port/test/night --res-dir ./NIGHT \
      --script "$DRIVE" --rig-dump "$OUT/port" --rig-hz 10 --frames $((DUR * 60)) > "$OUT/port.log" 2>&1 )
say "port: done ($(wc -c < "$OUT/port/cells.bin" 2>/dev/null || echo 0) bytes of cell records)"

# ---- original side, session 1: state + heap replay -------------------------------------
say "original: boot + calibrate + replay"
RUN_GAME=1 bash "$HERE/bringup.sh" > "$OUT/bringup1.log" 2>&1
sleep 10
timeout 120 python3 "$HERE/calibrate_ds.py" > "$OUT/calib1.log" 2>&1 || { say "calibrate failed"; exit 1; }
timeout $((DUR + 120)) python3 "$HERE/replay_original.py" --drive "$DRIVE" --out "$OUT/orig" \
        --seconds "$DUR" --hz 4 --audio-capture > "$OUT/replay.log" 2>&1
# the WAV lands inside the container; the mount is /work, so pull it out next to the state snapshots
NEWEST=$(docker exec rig bash -c 'ls -t /root/.config/dosbox-automation/capture/*.wav 2>/dev/null | head -1')
if [ -n "$NEWEST" ]; then docker cp "rig:$NEWEST" "$OUT/orig/audio.wav" && say "original: audio $(basename "$NEWEST") -> $OUT/orig/audio.wav"; fi
say "original: $(tail -1 "$OUT/replay.log")"

# ---- original side, session 2: screen capture (fresh boot, same drive) -----------------
say "original: boot + capture (screen)"
RUN_GAME=1 bash "$HERE/bringup.sh" > "$OUT/bringup2.log" 2>&1
sleep 10
# the capture session must be able to evaluate `wait ds` anchors too, so it needs its own calibration
timeout 120 python3 "$HERE/calibrate_ds.py" > "$OUT/calib2.log" 2>&1 || say "calibrate (session 2) failed"
timeout $((DUR + 90)) python3 "$HERE/capture_original.py" --out "$OUT/orig_screen" --seconds "$DUR" \
        --hz 25 --drive "$DRIVE" > "$OUT/capture.log" 2>&1
say "original: $(tail -1 "$OUT/capture.log")"
timeout 20 curl -s --noproxy '*' -X POST -H "Authorization: Bearer $(cat /tmp/rig/.token)" \
    http://localhost:8386/api/v1/control/shutdown >/dev/null 2>&1 || true

# ---- oracles ---------------------------------------------------------------------------
say "oracles"
timeout 120 python3 "$HERE/diff_state.py" --orig "$OUT/orig" --port "$OUT/port" --drive "$DRIVE" \
        --md "$OUT/report_state.md" > "$OUT/report_state.txt" 2>&1
timeout 120 python3 "$HERE/diff_activity.py" --orig "$OUT/orig" --port "$OUT/port" --drive "$DRIVE" \
        --md "$OUT/report_activity.md" > "$OUT/report_activity.txt" 2>&1
timeout 240 python3 "$HERE/diff_screen.py" --orig "$OUT/orig_screen/frames.bin" \
        --port "$OUT/port/frames.bin" --orig-run "$OUT/orig" --md "$OUT/report_screen.md" \
        > "$OUT/report_screen.txt" 2>&1
if [ -f "$OUT/orig/audio.wav" ] && [ -f "$OUT/port/pcm.wav" ]; then
  timeout 120 python3 "$HERE/diff_audio.py" --orig "$OUT/orig/audio.wav" --port "$OUT/port/pcm.wav" \
          --port-cells "$OUT/port/cells.bin" --drive "$DRIVE" --md "$OUT/report_audio.md" \
          > "$OUT/report_audio.txt" 2>&1
else
  say "audio oracle skipped (no captured WAV on one side)"
fi

{
  echo "# divergence report"
  echo
  echo "- drive: \`$DRIVE\`  duration ${DUR}s"
  echo "- generated: $(date -u +%FT%TZ)"
  echo "- port env: \`${PORT_ENV:-none}\`"
  echo
  for f in report_state.txt report_activity.txt report_screen.txt report_audio.txt; do
    echo "## $f"; echo '```'; cat "$OUT/$f"; echo '```'; echo
  done
} > "$OUT/DIVERGENCE.md"
say "wrote $OUT/DIVERGENCE.md"
grep -hE "^(mode=|samples:|tally|cells with|divergent runs|pacing check|channel-order|FP CONTROL|original:|port  |whole-route|  P[0-9])" \
     "$OUT"/report_state.txt "$OUT"/report_activity.txt "$OUT"/report_screen.txt || true

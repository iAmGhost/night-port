#!/usr/bin/env bash
# sweep_episodes.sh -- task-8: docs/59's nine-episode sweep, replayed on the Pocket code path.
#
# The SDL host's sweep (docs/59) is driven by `--script route_show_keyed.txt`, a keyed timeline with resource
# anchors.  The Pocket host has no keyed script and no anchors (docs/333 §5 says so outright); what it has is
# the warp entry pump, which is anchored on GAME STATE instead of wall clock: it posts a confirm pulse until
# fn_03272f (the player action resolver) starts owning frames, and stops there.  That is the route used here,
# and it is why this sweep is reproducible at all: 25 pulses got stage 1 live, and the pulse count is printed.
#
# Every stage dumps the rig's own witnesses (src/app/rig_sink.c, the same files the differential rig reads on
# the SDL side: frames.bin, cells.bin, records.jsonl, player_recs.jsonl), which is what makes the state half of
# the comparison possible -- until 0.1.53 the Pocket host dumped frames only.
#
#   FRAMES=6000 OUT=/tmp/pocket_sweep scripts/pocket/sweep_episodes.sh
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
FRAMES=${FRAMES:-6000}
OUT=${OUT:-/tmp/pocket_sweep}
APP=${APP:-$ROOT/pocket/src/night/app_pc}
mkdir -p "$OUT"
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
export NIGHT_SEED=1234 NIGHT_CD_DIR="$ROOT/scratch/pocket/cd"
export NIGHT_LOG=1 NIGHT_VIDRAW=1     # the arena bytes behind each picture: docs/62's basis
for n in 1 2 3 4 5 6 7 8 9; do
  d="$OUT/stage$n"
  rm -rf "$d"; mkdir -p "$d"
  start=$(date +%s)
  "$APP" --warp-stage "$n" --frames "$FRAMES" --rig-dump "$d" --rig-witness "${WITNESS:-0}" > "$d/run.log" 2>&1
  rc=$?
  echo "stage $n: exit=$rc wall=$(( $(date +%s) - start ))s dump=$d"
  echo "$rc" > "$d/exit.txt"
done
python3 "$ROOT/scripts/pocket/sweep_report.py" "$OUT"

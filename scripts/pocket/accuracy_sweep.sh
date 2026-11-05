#!/usr/bin/env bash
# Reproduce the whole picture-accuracy table of docs/382-383 in one command.
#
# Three Pocket-shim runs (the SDK's SDL2 desktop shim, docs/330), each witnessed in the arena's index space plus the
# game's own palette, decoded to BMPs, then scored per event window of an original-side capture against them:
#   warp      --warp-stage 1                 live stage play, the pause windows' stage backgrounds
#   warp+esc  the same plus route_esc_menu   the game's pause menu, measured over the menu box as well
#   scripted  --script route_pause_menu      boot, attract, .fli intro, title, slides
#
# Precondition: an original-side capture directory (frames + events.tsv), built with
#   scripts/rig/capture_container.py  then  scripts/playtest_orig_drive_container.py  (docs/379).
# OVERALL=1 runs the whole sweep; a single score costs about a minute of matching per window.
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APP=${APP:-$ROOT/pocket/src/night/app_pc}
ORIG=${ORIG:-/tmp/orig71}
OUT=${OUT:-/tmp/accuracy}
WIT=${WIT:-400}
FRAMES=${FRAMES:-6000}
WINDOW=${WINDOW:-}
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=${NIGHT_SEED:-1234}
export NIGHT_CD_DIR=${NIGHT_CD_DIR:-scratch/pocket/cd} NIGHT_LOG=${NIGHT_LOG:-1} NIGHT_SAVE_DIR=${NIGHT_SAVE_DIR:-/tmp/svrt}
export NIGHT_VIDRAW=1 NIGHT_VIDRAW_MAX=$WIT
[ -f "$ORIG/events.tsv" ] || { echo "no $ORIG/events.tsv -- capture the original side first (docs/379)"; exit 1; }
(cd "$ROOT/pocket/src/night" && make app_pc >/dev/null 2>&1) || { echo "app_pc build failed"; exit 1; }

run() {  # run <name> <frames> <args...>
  local name=$1 frames=$2; shift 2; local d="$OUT/$name"
  rm -rf "$d"; mkdir -p "$d"
  "$APP" --frames "$frames" --rig-dump "$d" "$@" > "$d/run.log" 2>&1 || { echo "$name: run failed"; tail -5 "$d/run.log"; exit 1; }
  pyrun "$ROOT/scripts/pocket/arena_raws_to_bmp.py" --in "$d" --out "$d/bmp" | tail -1
  echo "  $name: $(ls "$d/arena" | wc -l | tr -d ' ') witnesses, $(grep -cE '^view=' "$d/run.log") view samples, $(grep -oE 'view=[01] [0-9]+x[0-9]+' "$d/run.log" | sort -u | tr '\n' ' ')"
  # Reported as a measurement, not as a verdict.  docs/382 inferred "no animation at all" from chg > 2000 and used it
  # to explain a 0/55; the warp run here has no sample above that threshold either, yet its stage frames match the
  # original's gameplay frames at 0.9998 -- so chg counts changed pixels between the host's *own* view samples, not
  # between the game and a reference, and it says far less about animation than docs/382 assumed.
  local mx=$(grep -oE 'chg=[0-9]+' "$d/run.log" | cut -d= -f2 | sort -n | tail -1)
  local hi=$(grep -oE 'chg=[0-9]+' "$d/run.log" | cut -d= -f2 | awk '$1>1000' | wc -l | tr -d ' ')
  echo "  $name: view-sample chg max=${mx:-0}, samples above 1000 changed px: $hi"
}

pyrun() { command uv run --with numpy --with pillow --quiet python3 "$@"; }   # not named uv: a helper named uv
                                                                               # swallowed the `uv run ...` call in run()

echo "== runs =="
run warp      $FRAMES --warp-stage 1
run esc       $FRAMES --warp-stage 1 --script "$ROOT/scripts/playtest/route_esc_menu.txt"
run scripted  12000   --script "$ROOT/scripts/playtest/route_pause_menu.txt"

echo; echo "== whole-frame score per original window =="
for w in boot title_melt title_card slides fli esc_win_ esc_win2_ gameplay stage_play; do
  printf '%-12s ' "$w"
  best=""
  for r in warp esc scripted; do
    v=$(pyrun "$ROOT/scripts/playtest_compare_orig_port.py" --port "$OUT/$r/bmp" --orig "$ORIG" --window "$w" 2>/dev/null | grep -oE 'NCC>=0.90: [0-9]+/[0-9]+' | sed 's/NCC>=0.90: //')
    [ -n "$v" ] && best="$best$r=$v  "
  done
  echo "${best:-no frames in this window}"
done

echo; echo "== the pause menu, measured over its own box (docs/383) =="
for w in esc_win_ esc_win2_; do
  printf '%-12s ' "$w"
  best=""
  for r in warp esc scripted; do
    v=$(pyrun "$ROOT/scripts/pocket/menu_region_compare.py" --port "$OUT/$r/bmp" --orig "$ORIG" --window "$w" 2>/dev/null | grep -E '^  menu region' | sed 's/.*>=0.90: //')
    [ -n "$v" ] && best="$best$r=$v  "
  done
  echo "${best:-n/a}"
done
echo; echo "docs/382 and docs/383 carry the expected shape of this table and what each zero means."

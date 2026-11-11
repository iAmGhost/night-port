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
# Accept a bare drive name ("probe_cd") as well as a path: the original-side replay passes --drive straight
# to a file open, so a bare name used to die with FileNotFoundError 'probe_cd' after the port half had
# already run (48 MB of cell records wasted).
if [ ! -f "$DRIVE" ] && [ -f "$HERE/drives/$DRIVE.txt" ]; then DRIVE="$HERE/drives/$DRIVE.txt"; fi
if [ ! -f "$DRIVE" ]; then echo "route.sh: no such drive: $1 (nor $HERE/drives/$1.txt)" >&2; exit 2; fi
DUR=${2:-30}
OUT=${OUT:-/tmp/rig/route-$(date +%Y%m%d-%H%M%S)}
# The port's audio ladder opens the host default device and then walks every alternate device. On a host
# where CoreAudio refuses devices slowly (measured: ~5 MINUTES per refused device on this machine), a rig
# run can spend longer in audio probing than the whole drive. The rig witnesses (board PCM and the CD-DA
# stream) are tee'd before the device, so a dummy driver costs nothing analytically: default to it, and let
# PORT_ENV override for runs that genuinely want the host device.
# docs/167 §7: NIGHT_SEED is pinned for every rig run.  Without it the port seeds its LCG from a timer
# (fn_03af18 / 0x03af59 -> fn_04297a), and two identical 4000-frame runs of the SAME binary and drive
# differed: 844 vs 816 fn_03272f entries, idle-timer ending at 35 vs 13.  Port-side oracle numbers were
# therefore carrying run-to-run variance until this line existed.
# docs/167 §21 clock calibration: the shipped frame-counter cell DS 0x63360 advances 9 or 10 quanta per 100 ms
# (933 steps of 9, 789 of 10 on g15).  The port's default wall-clock pump gives 7 (677 of 1297), i.e. the port
# runs its game clock ~26% slow, and every timed threshold in a comparison is then skewed.  NIGHT_RIG_STEP_CALLS
# is the PIT-handler calls per second the fixed-step mode drives; 490 reproduces the 7-quantum behaviour, 665
# reproduces shipped's distribution (346 steps of 9, 346 of 10), 700 overshoots (703 of 10).  The oracle runs at
# the calibrated cadence so both sides tick at the same rate; the port's unpinned real-time loop staying at 7 is
# a separate, real pacing defect and is filed as such.
PORT_ENV=${PORT_ENV:-SDL_AUDIODRIVER=dummy NIGHT_SEED=${RIG_SEED:-13579} NIGHT_RIG_STEP_CALLS=${RIG_STEP:-665} NIGHT_RIG_FPS=${RIG_FPS:-60}}
mkdir -p "$OUT"
say() { printf '%s  %s\n' "$(date -u +%H:%M:%S)" "$*"; }

say "route: drive=$DRIVE duration=${DUR}s out=$OUT"

# ---- port side -------------------------------------------------------------------------
# The rig must never run a binary older than the sources it is meant to be testing: docs/133 §8 caught a
# before/after comparison made against a stale ./build/night, which silently reported the *old* values
# for cells whose initializers had just been fixed. Build first, then record the stamp in the run dir.
say "port: build (app target)"
if ! ( cd "$ROOT" && make app ) > "$OUT/build.log" 2>&1; then
    say "port: BUILD FAILED (see $OUT/build.log)"; exit 1
fi
if [ -n "$(find src test/test_f1.c Makefile -newer build/night 2>/dev/null | head -1)" ]; then
    # make compares timestamps to one-second granularity, so an edit that lands in the same second as
    # the link leaves the target "up to date" while find -newer (sub-second) still reports the source
    # as newer -- the guard tripped spuriously on two runs (docs/266). Resolve it by rebuilding the app
    # target from scratch, then re-check; only give up if the binary is still older than a source.
    say "port: guard tripped, rebuilding app target with -B"
    ( cd "$ROOT" && make app -B ) >> "$OUT/build.log" 2>&1 || { say "port: REBUILD FAILED"; exit 1; }
    if [ -n "$(find src test/test_f1.c Makefile -newer build/night 2>/dev/null | head -1)" ]; then
        say "port: WARNING a source is still newer than build/night after -B"; exit 1
    fi
fi
printf 'binary=%s mtime=%s\n' "build/night" "$(stat -f '%Sm' build/night 2>/dev/null || stat -c '%y' build/night)" > "$OUT/build_stamp.txt"
say "port: night --rig-dump"
( cd "$ROOT" && env $PORT_ENV timeout $((DUR + 40)) ./build/night \   # --res-dir defaults to ./extracted, what `make extract` builds (docs/455)
      --script "$DRIVE" --rig-dump "$OUT/port" --rig-hz 10 --frames $((DUR * 60)) > "$OUT/port.log" 2>&1 )
say "port: done ($(wc -c < "$OUT/port/cells.bin" 2>/dev/null || echo 0) bytes of cell records)"

# ---- original side, session 1: state + heap replay -------------------------------------
# REUSE_ORIG=<previous route out dir> takes the shipped halves (orig/, orig_screen/) from an earlier
# route instead of booting containers.  The shipped side is pinned (cycles 120000, core normal, fixed
# drive -- docs/100), so a cached capture is a legitimate comparison basis.  On 2026-11-03 the OrbStack
# engine stopped answering `docker run` / `docker exec` at all (40 s and 400 s timeouts, rc=124, no
# output; port 8386 still forwarded by OrbStack but the API answers nothing), so this is the only way to
# keep the canonical-route oracle running.  The reuse is stamped in REUSE_ORIGIN.txt and recorded in
# docs/308: a reused shipped half measures PORT change against a FIXED shipped reference, which is what a
# regression gate needs -- it does not re-verify the shipped side itself.
if [ -n "${REUSE_ORIG:-}" ]; then
  for sub in orig orig_screen; do
    if [ ! -d "$REUSE_ORIG/$sub" ]; then say "REUSE_ORIG: $REUSE_ORIG/$sub missing"; exit 2; fi
    cp -R "$REUSE_ORIG/$sub" "$OUT/$sub" || { say "REUSE_ORIG: copy failed"; exit 2; }
  done
  {
    echo "reused shipped halves from: $REUSE_ORIG"
    echo "  state dumps:  $(ls "$OUT/orig"/state_*.ds.bin 2>/dev/null | wc -l)"
    echo "  screen frames: $(find "$OUT/orig_screen" -name '??????_*.bin' 2>/dev/null | wc -l)"
    echo "  audio wav:     $(ls "$OUT/orig"/audio.wav 2>/dev/null || echo none)"
    echo "  note: cached deterministic shipped capture, NOT a fresh container run (docs/308)"
  } > "$OUT/REUSE_ORIGIN.txt"
  say "original: REUSED from $REUSE_ORIG (no container booted -- see REUSE_ORIGIN.txt)"
else
say "original: boot + calibrate + replay"
RUN_GAME=1 bash "$HERE/bringup.sh" > "$OUT/bringup1.log" 2>&1
sleep 10
timeout 120 python3 "$HERE/calibrate_ds.py" > "$OUT/calib1.log" 2>&1 || { say "calibrate failed"; exit 1; }
# RIG_ENTS=N walks the entity list (+0x13f) on the shipped side, matching the port dumper's ent## cells:
# entity-level oracles (speed model, hurt band, EXP gates) need it on BOTH sides or they compare nothing.
ENTS=""
# Default to the port dumper's slot count: without it the shipped side has no ent## cells and every
# entity-level question (docs/136 §7's reaction-code comparison, the EXP gate tuple, the +0x30 dying
# flag) is unanswerable from a route. RIG_ENTS=0 still turns it off.
if [ -z "${RIG_ENTS+x}" ]; then RIG_ENTS=12; fi
if [ -n "${RIG_ENTS:-}" ] && [ "$RIG_ENTS" != 0 ]; then ENTS="--ents $RIG_ENTS"; fi
timeout $((DUR + 120)) python3 "$HERE/replay_original.py" --drive "$DRIVE" --out "$OUT/orig" \
        --seconds "$DUR" --hz ${RIG_HZ:-4} --audio-capture $ENTS > "$OUT/replay.log" 2>&1
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
        --hz 25 --drive "$DRIVE" ${RIG_IO:+--io "$RIG_IO"} > "$OUT/capture.log" 2>&1
say "original: $(tail -1 "$OUT/capture.log")"
timeout 20 curl -s --noproxy '*' -X POST -H "Authorization: Bearer $(cat /tmp/rig/.token)" \
    http://localhost:8386/api/v1/control/shutdown >/dev/null 2>&1 || true
fi   # end of the REUSE_ORIG / live-container branch

# ---- oracles ---------------------------------------------------------------------------
say "oracles"
timeout 120 python3 "$HERE/diff_state.py" --orig "$OUT/orig" --port "$OUT/port" --drive "$DRIVE" \
        --md "$OUT/report_state.md" > "$OUT/report_state.txt" 2>&1
timeout 120 python3 "$HERE/diff_activity.py" --orig "$OUT/orig" --port "$OUT/port" --drive "$DRIVE" \
        --md "$OUT/report_activity.md" > "$OUT/report_activity.txt" 2>&1
timeout 240 python3 "$HERE/diff_screen.py" --orig "$OUT/orig_screen/frames.bin" \
        --port "$OUT/port/frames.bin" --orig-run "$OUT/orig" --md "$OUT/report_screen.md" \
        > "$OUT/report_screen.txt" 2>&1
# The port writes TWO audio witnesses: port/pcm.wav (the sound board -- SFX) and port/cdda.raw
# (the CD-DA stream, 44100 Hz stereo, the exact bytes handed to the device). The shipped mixer
# capture contains both mixed together, so the oracle has to be run against each stream in turn:
# against pcm.wav only, every CD-DA fingerprint came back NOT FOUND (docs/113).
if [ -f "$OUT/orig/audio.wav" ] && [ -f "$OUT/port/pcm.wav" ]; then
  timeout 120 python3 "$HERE/diff_audio.py" --orig "$OUT/orig/audio.wav" --port "$OUT/port/pcm.wav" \
          --port-cells "$OUT/port/cells.bin" --port-log "$OUT/port.log" --drive "$DRIVE" \
          --md "$OUT/report_audio_sfx.md" \
          > "$OUT/report_audio_sfx.txt" 2>&1
  say "audio oracle: board/SFX stream"
fi
if [ -f "$OUT/orig/audio.wav" ] && [ -f "$OUT/port/cdda.raw" ]; then
  timeout 60 python3 "$HERE/wav_wrap.py" "$OUT/port/cdda.raw" "$OUT/port/cdda.wav" > "$OUT/wav_wrap.txt" 2>&1
  timeout 180 python3 "$HERE/diff_audio.py" --orig "$OUT/orig/audio.wav" --port "$OUT/port/cdda.wav" \
          --port-log "$OUT/port.log" --drive "$DRIVE" --md "$OUT/report_audio_cd.md" \
          > "$OUT/report_audio_cd.txt" 2>&1
  say "audio oracle: CD-DA stream"
fi
if [ ! -f "$OUT/report_audio_sfx.txt" ] && [ ! -f "$OUT/report_audio_cd.txt" ]; then
  say "audio oracle skipped (no captured WAV on one side)"
fi

{
  echo "# divergence report"
  echo
  echo "- drive: \`$DRIVE\`  duration ${DUR}s"
  echo "- generated: $(date -u +%FT%TZ)"
  echo "- port env: \`${PORT_ENV:-none}\`"
  echo
  for f in report_state.txt report_activity.txt report_screen.txt report_audio_sfx.txt report_audio_cd.txt; do
    [ -f "$OUT/$f" ] || continue      # docs/287 §4: the summary used to cat a report_audio.txt that no leg writes
    echo "## $f"; echo '```'; cat "$OUT/$f"; echo '```'; echo
  done
} > "$OUT/DIVERGENCE.md"
say "wrote $OUT/DIVERGENCE.md"
grep -hE "^(mode=|samples:|tally|cells with|divergent runs|pacing check|channel-order|FP CONTROL|original:|port  |whole-route|  P[0-9])" \
     "$OUT"/report_state.txt "$OUT"/report_activity.txt "$OUT"/report_screen.txt || true

#!/bin/bash
# docs/91: the long soak. Runs every episode (--warp-stage 1..9) under a hard per-stage timeout, first in the
# plain build (for the arena / RSS growth table) then in the ASan build (for "no ASan report"), and prints one
# summary line per stage from the host's `memstat` witness.
#
#   scripts/playtest_soak.sh [plain|asan|both]      (default: both)
#
# Budget: each stage runs --frames $FRAMES (default 4600 ~= 66 s of game time: the boot chain takes ~52 s before
# the room is live, so this is ~14 s of room play). The ASan build runs at roughly 0.2x real time, so allow
# ~5 min per stage there.
set -u
cd "$(dirname "$0")/../port" || exit 1
PHASE="${1:-both}"
FRAMES="${FRAMES:-4600}"
OUT="${OUT:-/tmp/night_soak}"
mkdir -p "$OUT"
SCRIPT=../scripts/playtest/route_walk_fight.txt
ENVS="SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy"

run_stage() {                    # $1 = stage, $2 = log file, $3 = asan env prefix
    local st="$1" log="$2" asan="$3" to="$4"
    env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy $asan \
        timeout "$to" ./test/night --warp-stage "$st" --script "$SCRIPT" --no-audio \
        --frames "$FRAMES" --shot-limit 2 > "$log" 2>&1
    echo "stage $st rc=$?"
}

report() {                       # $1 = stage, $2 = log
    local st="$1" log="$2"
    local first last asan_errs frames room
    first=$(grep -m1 "^.*memstat" "$log" | sed 's/.*memstat //')
    last=$(grep "memstat" "$log" | tail -1 | sed 's/.*memstat //')
    asan_errs=$(grep -cE "ERROR: AddressSanitizer|SEGV" "$log")
    frames=$(grep -o "frames presented: [0-9]*" "$log" | tail -1)
    room=$(grep -cE "shot .*(mid_show|after_show|walk_|episode_start)" "$log")
    printf "%s\tfirst[%s]\tlast[%s]\asan_errs=%s\t%s\troomshots=%s\n" "$st" "$first" "$last" "$asan_errs" "$frames" "$room"
}

if [ "$PHASE" = plain ] || [ "$PHASE" = both ]; then
    make -C . app >/dev/null 2>&1
    : > "$OUT/plain.tsv"
    for st in 1 2 3 4 5 6 7 8 9; do
        run_stage "$st" "$OUT/plain_stage$st.log" "" 300
        report "$st" "$OUT/plain_stage$st.log" | tee -a "$OUT/plain.tsv"
    done
fi

if [ "$PHASE" = asan ] || [ "$PHASE" = both ]; then
    make -C . app CFLAGS="-std=c11 -O1 -g -w -fsanitize=address" >/dev/null 2>&1
    : > "$OUT/asan.tsv"
    for st in 1 2 3 4 5 6 7 8 9; do
        run_stage "$st" "$OUT/asan_stage$st.log" "ASAN_OPTIONS=detect_leaks=0:halt_on_error=0" 360
        report "$st" "$OUT/asan_stage$st.log" | tee -a "$OUT/asan.tsv"
    done
    make -C . app >/dev/null 2>&1      # leave the normal build in place
fi

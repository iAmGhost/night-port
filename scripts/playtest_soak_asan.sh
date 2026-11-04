#!/bin/bash
# docs/91: phase 2 of the soak -- every episode under AddressSanitizer, each stage bounded by a hard `timeout`.
# Run after the plain phase (scripts/playtest_soak.sh plain) so the two sets of RSS numbers are not measured
# against each other.
#
#   ASAN_TIMEOUT=480 scripts/playtest_soak_asan.sh
#
# 480 s is deliberate: the ASan build runs at roughly 0.2x real time, and the boot chain alone costs ~52 s of
# game time before a room is live, so a shorter bound would only soak the title/attract path. The room witness
# (memstat's room= field) shows how many samples of each run were inside a live room.
set -u
cd "$(dirname "$0")/.." || exit 1
OUT="${OUT:-/tmp/night_soak}"
ASAN_TIMEOUT="${ASAN_TIMEOUT:-480}"
FRAMES="${FRAMES:-4600}"
SCRIPT=scripts/playtest/route_walk_fight.txt
mkdir -p "$OUT"

touch src/app/keymap.c src/app/main.c src/f/f.c src/f/fx_04794a.c src/sys/sys.c src/cd/cd_iso.c
make app CFLAGS="-std=c11 -O1 -g -w -fsanitize=address" >/dev/null 2>&1 || { echo "ASan build failed"; exit 1; }
: > "$OUT/asan.tsv"
for st in 1 2 3 4 5 6 7 8 9; do
    env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        ASAN_OPTIONS=detect_leaks=0:halt_on_error=0:abort_on_error=0 \
        timeout "$ASAN_TIMEOUT" ./build/night --warp-stage "$st" --script "$SCRIPT" --no-audio \
        --frames "$FRAMES" --shot-limit 2 > "$OUT/asan_stage$st.log" 2>&1
    rc=$?
    n=$(grep -cE "ERROR: AddressSanitizer|runtime error: |\bSEGV\b" "$OUT/asan_stage$st.log")
    last=$(grep memstat "$OUT/asan_stage$st.log" | tail -1 | sed 's/.*memstat //')
    echo "stage $st rc=$rc asan_reports=$n last[$last]" | tee -a "$OUT/asan.tsv"
done
touch src/app/keymap.c src/app/main.c src/f/f.c src/f/fx_04794a.c src/sys/sys.c src/cd/cd_iso.c
make app >/dev/null 2>&1      # leave the normal build in place

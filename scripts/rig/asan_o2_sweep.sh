#!/bin/sh
# docs/146 verification: run the stored fuzz corpus under the -O2 sanitizer build.
# Every run is hard-capped; nothing here can loop.
BIN=./build/night-asan-o2
OUT=/tmp/rig/asan_o2_sweep
mkdir -p "$OUT"
i=0
for d in /tmp/rig/fz16/*_st*; do
  [ -f "$d/drive.txt" ] || continue
  n=$(basename "$d")
  w=$(grep -cE "^(poke|ent|play) " "$d/drive.txt")
  st=$(echo "$n" | sed 's/.*st//')
  # the same family subset fuzz.py would have used for this seed dir (docs/120 §13: crc32, reproducible)
  fam=$(python3 - "$d" <<'PY'
import sys, zlib
pool=["ena-1","enc-1","end-1","ene-1","enf-1","eng-1","enh-1","eni-1","enb-1","enm-1","ens-1","enr-1","enl-1","mark","dust"]
k=zlib.crc32(sys.argv[1].encode())%15
print(",".join(pool[k:k+4] or pool[:4]))
PY
)
  timeout 200 env SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 NIGHT_WARP_ENTITY="$fam" NIGHT_WARP_ENTITY_AT=3000 \
    "$BIN" --script "$d/drive.txt" --warp-episode "$st" --rig-dump /tmp/rig/asan_o2_dump --rig-hz 1 --frames 5200 \
    > "$OUT/$n.log" 2>&1 &
  i=$((i+1))
  if [ $((i % 4)) -eq 0 ]; then wait; fi
done
wait
clean=0; bad=0
for f in "$OUT"/*.log; do
  n=$(basename "$f" .log)
  rc=$(grep -cE "ERROR: AddressSanitizer|runtime error|SUMMARY: AddressSanitizer" "$f")
  fin=$(grep -c "frames presented" "$f")
  rej=$(grep -c "rig: poke" "$f")
  if [ "$rc" -eq 0 ] && [ "$fin" -gt 0 ]; then clean=$((clean+1)); else bad=$((bad+1)); fi
  printf "%-12s sanitizer=%s ended=%s pokes_rejected=%s\n" "$n" "$rc" "$fin" "$rej"
done
echo "== clean=$clean not-clean=$bad total=$((clean+bad)) =="

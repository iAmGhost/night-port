#!/usr/bin/env bash
# ab.sh -- run the four hardware questions this port still has open, one command each (docs/372, docs/369, docs/374).
#
# Every remaining open task needs the same thing: a boot on the Pocket and its nightlog.sav.  What was missing is a
# way to set up each variant without hand-editing the ini or guessing the build flags -- which is how a "measure the
# device" task quietly becomes a fifteen-minute chore and gets skipped.
#
#   scripts/pocket/ab.sh player      back to the shipping shape (no log, no probes, no panel)
#   scripts/pocket/ab.sh diag        LOG=1 so the budget/stages/perf lines exist
#   scripts/pocket/ab.sh bram-pump   tick_pump in BRAM instead of the copy path (docs/372 set B)
#   scripts/pocket/ab.sh audio-tick  the board DMA drain gated to the 490 Hz tick (docs/369 5)
#   scripts/pocket/ab.sh show        the card after any boot: version, md5, ini, and the perf lines
#
# Each variant prints the grep to run after the boot.  Nothing here rewrites the objective or the shipping defaults:
# `player` is what a player's card looks like, and the last command of any session should be `player`.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
CARD=${CARD:-/Volumes/Pocket}
INI="$CARD/Assets/night/common/night.ini"
ELF="$CARD/Assets/night/common/night.elf"

need_card() { [ -d "$CARD" ] || { echo "no card mounted at $CARD"; exit 1; }; }
ini_set() { python3 - "$INI" "$1" "$2" <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1]); s = p.read_text()
k, v = sys.argv[2], sys.argv[3]
if re.search(r'(?m)^%s=.*$' % k, s):
    s = re.sub(r'(?m)^%s=.*$' % k, f'{k}={v}', s)
else:
    s = s.rstrip('\n') + f'\n{k}={v}\n'
p.write_text(s)
PY
}
report() {
  echo "after the boot: eject, mount, then"
  echo "  grep -E '^(sdk|cdda|stages|perf|budget|audio|disc|witness|save|load)' \\"
  echo "    \"$CARD/Saves/night/common/nightlog.sav\" | tail -40"
  echo "and to identify the card you actually ran:"
  echo "  strings \"$ELF\" | grep -E '^0\\.1\\.'; md5 -q \"$ELF\""
}

case "${1:-}" in
  player)
    need_card
    for kv in SCREEN=0 DIAG=0 LOG=0 PUMP_AUDIO_TICK=0 RESTORE=1; do ini_set "${kv%%=*}" "${kv##*=}"; done
    echo "player shape written:"; grep -nE '^(SCREEN|DIAG|LOG|PUMP_AUDIO_TICK|RESTORE|FB_MODE|BLIT_DIRTY|DISC_ASYNC)=' "$INI"
    ;;
  diag)
    need_card
    ini_set LOG 1; ini_set DIAG 1
    echo "diagnostics on (LOG=1 DIAG=1). Now the budget line carries the device's SD timings."
    report
    ;;
  bram-off)
    need_card
    ( cd "$ROOT/pocket/src/night" && rm -f .obj/night/main_of.o .obj/night/of_cdda.o && make BRAM_SET=none >/dev/null && make BRAM_SET=none dist >/dev/null && make BRAM_SET=none deploy >/dev/null )
    ini_set LOG 1; ini_set DIAG 0   # after the deploy: deploy.sh rewrites night.ini (docs/395)
    echo "BRAM placement removed (BRAM_SET=none): nothing in APP_BRAM, and no noinline/optimize attribute on the hot host path."
    echo "  this is the shape every build before docs/371 had -- compare the feel against the BRAM build on the same stretch."
    report
    ;;
  disc-simple)
    need_card
    ini_set LOG 1
    for kv in DISC_BATCH=1 DISC_MIN_WANT=1 DISC_PREFILL_FIRST=0; do ini_set "${kv%%=*}" "${kv##*=}"; done
    echo "pre-docs/375 disc shape (one sector per read, no chunk gate, no prefill slice). If the music delay goes away, the batching policy is the cause."
    report
    ;;
  disc-fast)
    need_card
    for kv in DISC_BATCH=12 DISC_MIN_WANT=12 DISC_PREFILL_FIRST=150; do ini_set "${kv%%=*}" "${kv##*=}"; done
    echo "shipping disc shape restored (12/12/150)."
    ;;
  bram-pump)
    need_card
    ( cd "$ROOT/pocket/src/night" && rm -f .obj/night/main_of.o .obj/night/of_cdda.o && make BRAM_SET=pump >/dev/null && make BRAM_SET=pump dist >/dev/null && make BRAM_SET=pump deploy >/dev/null )
    ini_set LOG 1; ini_set DIAG 0   # after the deploy: deploy.sh rewrites night.ini (docs/395)
    echo "BRAM_SET=pump deployed (tick_pump + the drain path in zero-wait RAM, the copy path back in SDRAM)."
    echo "  compare its 'stages ... copy=' column against the same route on the shipping set -- that column is the copy."
    report
    ;;
  audio-tick)
    need_card
    ini_set LOG 1; ini_set PUMP_AUDIO_TICK 1
    echo "PUMP_AUDIO_TICK=1 set (docs/369 5). Judge it on the budget line's sfx plays/drops/steals and dry staying flat."
    report
    ;;
  picture-hardware)
    need_card
    for kv in FB_MODE=0 BLIT_DIRTY=0 PRESENT_SYNC=0 FIT=1 LOG=1 DIAG=0 SCREEN=0; do ini_set "${kv%%=*}" "${kv##*=}"; done
    echo "FB_MODE=0: OS 320x240 surface + software 2x2 downscale. THIS IS THE LOGO-FREEZE SHAPE (docs/401, docs/405)."
    echo "  82 657 us per present against 15 771 us for FB_MODE=1: 0.93 MB/s against 19.5 MB/s. 0.1.84/0.1.85 shipped"
    echo "  it on a geometry theory and the logo stuck again within one boot. Only boot this to study the geometry."
    echo "  take a screenshot of the TITLE screen (it is a 640x480-mode card) and compare against analysis/refs/pocket_0_1_83_title_corrupted.png."
    report
    ;;
  picture-native)
    need_card
    for kv in FB_MODE=1 BLIT_DIRTY=0 PRESENT_SYNC=0 FIT=1 LOG=1 DIAG=0 SCREEN=0; do ini_set "${kv%%=*}" "${kv##*=}"; done
    echo "FB_MODE=1: native 640x480 surface + row memcpy -- THE SHIPPING SHAPE from 0.1.86 (19.5 MB/s, docs/401/405)."
    echo "  the doubled title that made me doubt this path was the stale-surface commit (docs/404 5), which lives"
    echo "  only in this branch and is fixed; this is the shape whose one known defect is understood."
    report
    ;;
  picture-scaler)
    need_card
    for kvin in FIT=2 FB_MODE=0; do ini_set "${kvin%%=*}" "${kvin##*=}"; done
    echo "ask the CORE to scale (FIT=2): the OS scaler does the 320x200 -> panel stretch instead of my software loop."
    echo "  the untried lever for the copy cost: the aperture is at least 19.5 MB/s, so the cost is my downscale, not the write."
    report
    ;;
  disc-async)
    need_card
    ini_set LOG 1; ini_set DISC_ASYNC 1
    echo "async double-buffered SD prefetch back on (docs/403). It is off by default because the 0.1.83 boot said no:"
    echo "  two reads issued, mean_op=0us -- the completion never arrived, so no stage ever held landed bytes."
    echo "  0.1.84 falls back to synchronous reads instead of returning empty, so this can no longer mute the music;"
    echo "  judge it on 'cdda async waits' climbing while dry stays 0, not on music playing."
    report
    ;;
  show)
    need_card
    echo "core:  $(grep -o '"version": *"[0-9.]*"' "$CARD/Cores/iamghost.night/core.json" | head -1)"
    echo "elf:   $(strings "$ELF" | grep -E '^0\.1\.' | head -1)  md5 $(md5 -q "$ELF")"
    echo "ini:"; grep -nE '^[A-Z_]+=' "$INI" | sed 's/^/   /'
    ;;
  *)
    sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
    ;;
esac

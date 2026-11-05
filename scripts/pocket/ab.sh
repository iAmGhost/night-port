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
  echo "  grep -E '^(stages|perf|budget|audio|disc|witness|save|load)' \\"
  echo "    \"$CARD/Saves/night/common/nightlog.sav\" | tail -40"
  echo "and to identify the card you actually ran:"
  echo "  strings \"$ELF\" | grep -E '^0\\.1\\.'; md5 -q \"$ELF\""
}

case "${1:-}" in
  player)
    need_card
    for kv in SCREEN=0 DIAG=0 LOG=0 PUMP_AUDIO_TICK=0 RESTORE=1; do ini_set "${kv%%=*}" "${kv##*=}"; done
    echo "player shape written:"; grep -nE '^(SCREEN|DIAG|LOG|PUMP_AUDIO_TICK|RESTORE)=' "$INI"
    ;;
  diag)
    need_card
    ini_set LOG 1; ini_set DIAG 1
    echo "diagnostics on (LOG=1 DIAG=1). Now the budget line carries the device's SD timings."
    report
    ;;
  bram-pump)
    need_card
    ini_set LOG 1; ini_set DIAG 0
    ( cd "$ROOT/pocket/src/night" && make dist >/dev/null && make BRAM_SET=pump >/dev/null && make deploy >/dev/null )
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

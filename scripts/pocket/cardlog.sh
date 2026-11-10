#!/bin/sh
# Read everything a Pocket boot leaves on the card, in one command (docs/393).
#
# The point is the loop: boot the core, play, eject, plug the card in here, run this.  It prints the core's identity,
# the ini the core actually ran with, any diagnostic trail the core wrote, and the save slots -- so a hardware answer
# (frame cost, disc read cost, mixer pool, BRAM placement, overlay legibility) needs no screen photograph.
#
# Diagnostic lines are opt-in: night.ini DIAG= and LOG= (docs/362, docs/368).  A shipping LOG=0 boot leaves no trail,
# which is the correct default; scripts/pocket/ab.sh diag sets them for the measuring boot.
set -u
CARD="${CARD:-/Volumes/Pocket}"
[ -d "$CARD" ] || { echo "no card at $CARD (plug it in, or CARD=/path/to/card $0)"; exit 1; }
echo "== core identity =="
CORE="$CARD/Cores/iamghost.night/core.json"
if [ -f "$CORE" ]; then
  grep -o '"version": *"[^"]*"' "$CORE" | head -1 | sed 's/^/  core.json   /'
else echo "  core.json missing"; fi
ELF="$CARD/Assets/night/common/night.elf"
if [ -f "$ELF" ]; then
  echo "  elf version string: $(strings "$ELF" 2>/dev/null | grep -E '^0\.1\.' | head -1)"
  echo "  elf md5:            $(md5 -q "$ELF" 2>/dev/null || md5sum "$ELF" | cut -d' ' -f1)"
  echo "  build md5:          $(md5 -q "$(dirname "$0")/../../pocket/build/pocket/night/Assets/night/common/night.elf" 2>/dev/null || echo '(no build output; run: make -C pocket/src/night dist)')"
else echo "  night.elf missing"; fi
echo; echo "== night.ini the core ran with =="
INI="$CARD/Assets/night/common/night.ini"
if [ -f "$INI" ]; then
  grep -E "^(SCREEN|DIAG|LOG|FIT|FB_MODE|RESTORE|PUMP_AUDIO_TICK|PRESENT_SYNC|TICK_HZ|MASTER_VOL|SFX_GAIN|CDDA_GAIN|DISC_BATCH|DISC_MIN_WANT|DISC_PREFILL_FIRST|BRAM_SET|PERIPH)=" "$INI" | sed 's/^/  /'
  echo "  ($(grep -cE '^[A-Z_]+=' "$INI") keys total)"
else echo "  night.ini missing"; fi
echo; echo "== diagnostic trail =="
found=0
for f in "$CARD"/Saves/night/common/nightlog.sav "$CARD"/Saves/night/iamghost.night/nightlog.sav; do
  [ -f "$f" ] || continue
  sz=$(wc -c < "$f" | tr -d ' ')
  if head -c 4 "$f" | grep -q NGSV; then echo "  $f ($sz B): a save blob, not a log"; continue; fi
  found=1
  echo "  --- $f ($sz B)"
  tr -d '\r' < "$f" | sed 's/^/  | /'
done
[ "$found" = 1 ] || echo "  no diagnostic trail on the card: a shipping boot runs LOG=0 (docs/362). Set LOG=1 (or run scripts/pocket/ab.sh diag) on the measuring boot."
echo; echo "== save slots =="
n=0
for f in "$CARD"/Saves/night/common/night_[0-6].sav; do
  [ -f "$f" ] || continue; n=$((n+1))
  sz=$(wc -c < "$f" | tr -d ' ')
  if head -c 4 "$f" | grep -q NGSV; then
    printf "  %-14s %4s B  NGSV blob: " "$(basename "$f")" "$sz"
    od -An -tu1 -j6 -N2 "$f" | tr -s ' ' | sed 's/^ *//' | { read st; echo "stage=$st"; }
  else
    printf "  %-14s %4s B  (text or partial)\n" "$(basename "$f")" "$sz"
  fi
done
[ "$n" = 0 ] && echo "  none yet (L1+Select writes one: docs/392)"
echo; echo "== screenshots the boot left (Memories/Screenshots, newest 5) =="
ls -t "$CARD"/Memories/Screenshots/*.png 2>/dev/null | head -5 | sed 's/^/  /' || echo "  none"

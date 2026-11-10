#!/usr/bin/env bash
# Put a deployed card's night.ini into the instrumented state a diagnosis needs.
#
# WHY THIS EXISTS (2026-11-11, the morning of 0.2.5).  deploy.sh regenerates night.ini from the dist tree,
# and the dist tree ships quiet on purpose -- DIAG=0, LOG=0 -- because a player's core must not do MMIO or
# write to the card.  That is right for a release and silently wrong for me: after the 0.2.5 deploy the card
# sat at DIAG=0 LOG=0 DISC_IDLE_HOOK=0 CDDA_STOP_FIX=1, i.e. the next boot would have produced NO log at
# all, the idle-hook counter would have been pinned at zero for a second time for the wrong reason, and I
# would have spent another turn blaming the build.  The knobs a measurement needs are not the knobs a
# release ships with, so saying "I re-set them by hand every deploy" is not a process -- this is.
#
#   ./scripts/pocket/devini.sh [--card /Volumes/Pocket]
#
# Idempotent, ini-only, no rebuild, no deploy: run it AFTER deploy.sh (which overwrites it) and before
# ejecting.  verify_card.py reports the values so the state is on the record either way.
set -euo pipefail

SDCARD="${1:-/Volumes/Pocket}"; [ "${1:-}" = "--card" ] && SDCARD="${2:-/Volumes/Pocket}"
INI="$SDCARD/Assets/night/common/night.ini"

# key=value pairs this diagnosis needs on the device.  Keep the list short and each entry justified:
#   LOG=1              the run must be readable afterwards; the slot is a fixed 256 KB ring (docs/402)
#   DIAG=160           32 surfhash (one full-surface pass at boot) + 128 filecaps (one-shot OS caps)
#   DISC_IDLE_HOOK=1   keep the hook's presents= counter as the evidence that the OS calls it or not
#   ROW_SKIP=1         per-display-buffer row hashes -- the 640x480 copy is 70% of the machine (docs/429)
#   CDDA_STOP_FIX=0    docs/421/413: the game has no CD-DA stop, so do not let the port's bookkeeping
#                      delete 2.5 s of music.  The RELEASE default is still 1; that open choice belongs to
#                      a documented decision, not to a diagnosis, so it is not flipped in mk_dist.py here.
KNOBS=(
  "LOG=1"
  "DIAG=160"
  "DISC_IDLE_HOOK=1"
  "ROW_SKIP=0"    # MEASURED A LOSS on 0.2.8 hardware (30.1 ms static frame, 85% rows skipped): keep off
#                    # unless a fast-hash build (0.2.9+) beats ROW_SKIP=0 on the same route
  "CDDA_STOP_FIX=0"
)

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; RESET='\033[0m'
[ -f "$INI" ] || { echo -e "${RED}FAIL${RESET} no night.ini at $INI -- deploy first"; exit 1; }

changed=0
for kv in "${KNOBS[@]}"; do
    k="${kv%%=*}"; v="${kv#*=}"
    cur=$(awk -F= -v k="$k" '$1==k {gsub(/[ \t\r]/,"",$2); print $2; exit}' "$INI")
    if [ -z "$cur" ]; then
        echo -e "${RED}FAIL${RESET} $k is not a key in $INI -- mk_dist.py changed shape; do not guess"; exit 1
    fi
    if [ "$cur" = "$v" ]; then
        echo "  $k=$v (already)"
    else
        sed -i '' -e "s|^$k=.*|$k=$v|" "$INI"
        echo -e "${YELLOW}  $k: $cur -> $v${RESET}"; changed=$((changed + 1))
    fi
done

# The instrumented state is only ever wanted on a test card; a release deploy must stay quiet.  Print what
# is set so the person ejecting can see it, and warn that shipping these values would be a bug.
echo "  other knobs: $(grep -E '^(PRESENT_SKIP|DELAY_US|SCREEN|FDUMP|PALETTE_AFTER_FLIP|FB_MODE)=' "$INI" | tr '\n' ' ')"
if [ "$changed" = "0" ]; then echo -e "${GREEN}dev card state already set${RESET}"; else
    echo -e "${GREEN}dev card state set${RESET} ($changed key(s)); this is a TEST card -- do not ship it"; fi
echo "Now eject and boot."

#!/usr/bin/env bash
# Archive the card's run log BEFORE the next boot overwrites it.
#
# WHY.  nightlog.sav is a single fixed 256 KB slot: a fresh boot starts writing at the beginning, so by the
# time you mount the card you only ever have the LAST run (docs/402 is about reading that ring correctly;
# this is about not losing it).  That cost me a real answer on 2026-11-11: asked "did the logo wait get
# worse?", the only comparable measurement was in a log that the build in question had already overwritten.
# Every "did it regress?" question needs two logs, and one of them must be saved before the next boot.
#
#   ./scripts/pocket/logsave.sh            # archive the mounted card's log
#   ./scripts/pocket/logsave.sh --note pre-0.2.9
#
# Nothing here deletes or rewrites the card.
set -euo pipefail
SDCARD="/Volumes/Pocket"; NOTE=""
while [ $# -gt 0 ]; do
    case "$1" in
        --card) SDCARD="${2:-/Volumes/Pocket}"; shift 2;;
        --note) NOTE="-${2:-}"; shift 2;;
        *) echo "unknown arg: $1" >&2; exit 1;;
    esac
done
LOG="$SDCARD/Saves/night/common/nightlog.sav"
OUT="scratch/pocket/logs"; GREEN='\033[0;32m'; RED='\033[0m'; RESET='\033[0m'
[ -f "$LOG" ] || { echo -e "${RED}FAIL${RESET} no log at $LOG -- mount the card first"; exit 1; }
mkdir -p "$OUT"
# The core stamps its own version in the boot marker (docs/428), so the archive names itself.
VER=$(awk '/core [0-9]+\.[0-9]+\.[0-9]+: boot marker/ {gsub(/[^0-9.]/,"",$2); v=$2} END{print v}' "$LOG" 2>/dev/null)
[ -n "$VER" ] || VER=unknown
CARD_VER=$(sed -n 's/.*"version":[[:space:]]*"\([^"]*\)".*/\1/p' "$SDCARD/Cores/iamghost.night/core.json" 2>/dev/null | head -1)
NAME="$OUT/nightlog-${VER}-$(date +%Y%m%d-%H%M%S)${NOTE}.log"
cp "$LOG" "$NAME"
BYTES=$(wc -c < "$NAME" | tr -d ' ')
MARKS=$(grep -c "boot marker" "$NAME" 2>/dev/null || echo 0)
echo -e "${GREEN}archived${RESET} $NAME ($BYTES B, $MARKS boot marker(s), core $VER)"
# An archived log can only answer a question about the build that PRODUCED it. Every "did it regress?"
# comparison silently becomes a comparison against an older build if the boot ran old bits -- which happened
# on 2026-11-11: the card's night.elf was 0.2.24 (its strings carry "0.2.24" and " commits=%u"), the deploy
# was byte-verified, and the boot still printed "core 0.2.21: boot marker" in the pre-0.2.24 line layout.
# So: assert the banner against the card's own core.json, and say what is wrong rather than naming the file.
if [ -n "$CARD_VER" ] && [ "$VER" != "unknown" ] && [ "$VER" != "$CARD_VER" ]; then
    echo -e "${RED}VOID BOOT${RESET}: the log's banner says core $VER but the card's core.json says $CARD_VER."
    echo "  The machine did NOT run the deployed bits. Fix before measuring:"
    echo "    1. power the Pocket fully OFF (not sleep) -- a warm resume can relaunch a cached app"
    echo "    2. diskutil eject /Volumes/Pocket (a half-flushed exFAT write reads OLD bytes to the device)"
    echo "    3. re-run make -C pocket/src/night deploy && scripts/pocket/devini.sh, then boot"
    echo "  The archive is still written -- name it void, and never A/B against it."
    exit 1
fi
# The "commits=" assertion that used to sit here belongs to the 0.2.24 line: that build prints the
# commit counter on its split line and this one does not. Asserting it here would have screamed about
# a healthy 0.3.x boot -- a stale expectation is just another instrument that lies (docs/452).
# The VOID-banner guard above is the part worth keeping.
echo "  banner core $VER == card core.json $CARD_VER -- this boot tested what was deployed"
echo "next boot overwrites the card's copy -- run this again before it"

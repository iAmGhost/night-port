#!/bin/sh
# Restore the byte-exact player image taken from the card (docs/452, docs/453).
#
# WHY THIS EXISTS.  Rolling a Pocket core back has been a git operation, and git is not the card: a
# checkout can rebuild, and a rebuild can differ from what the hardware was actually given.  The
# player line is now judged by feel, so "put back the good build" must be a copy, not a build.
#
# This is that copy.  It restores Cores/<core> and Assets/<app> exactly as archived, verifies the
# version string that lands, and never touches Saves/ (logs, and whatever the game leaves behind).
#
#   scripts/pocket/restore_player.sh                 # restore to /Volumes/Pocket
#   scripts/pocket/restore_player.sh /Volumes/Other  # restore elsewhere
set -e
# The newest archive wins, so this never silently restores a stale build.
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC=$(ls -d "$ROOT"/scratch/pocket/player-* 2>/dev/null | sort -V | tail -1)
DST="${1:-/Volumes/Pocket}"
CORE="Cores/iamghost.night"
APP="Assets/night"

[ -n "$SRC" ] && [ -d "$SRC/$CORE" ] || { echo "no archived image at $SRC -- re-take it: see the bottom of this script"; exit 1; }
[ -d "$DST" ]       || { echo "$DST is not mounted"; exit 1; }

echo "restoring $SRC -> $DST"
for d in "$CORE" "$APP"; do
    mkdir -p "$DST/$d"
    rsync -a --delete --exclude '._*' --exclude '.DS_Store' "$SRC/$d/" "$DST/$d/"
done
sync
echo "card now reports core.json version: $(python3 -c "
import json,sys
d=json.load(open('$DST/$CORE/core.json'))
def walk(o):
    if isinstance(o,dict):
        for k,v in o.items():
            if k=='version' and isinstance(v,str): print(v); return
            walk(v)
    elif isinstance(o,list):
        for v in o: walk(v)
walk(d))")"
echo "then: python3 scripts/pocket/verify_card.py   # card vs build tree -- expect a MISMATCH here,"
echo "      because this restores the ARCHIVE, not whatever HEAD builds.  The archive is the truth."
echo
echo "Re-take the archive from a mounted card with:"
echo "  mkdir -p scratch/pocket/player-0.3.0 && rsync -a --exclude '._*' /Volumes/Pocket/Cores/iamghost.night" \\
echo "      scratch/pocket/player-0.3.0/Cores/ && rsync -a --exclude '._*' /Volumes/Pocket/Assets/night" \\
echo "      scratch/pocket/player-0.3.0/Assets/"

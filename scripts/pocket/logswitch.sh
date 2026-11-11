#!/bin/sh
# Turn the card log on or off -- and say what the card now says it is.
#
# WHY A SCRIPT AND NOT AN EDIT.  `LOG=` is the one ini key whose value decides whether a boot is
# observable at all, and two failures of this shape are on the record: deploy.sh rewrites night.ini
# from the dist tree, so a hand-set knob is silently reverted under you (docs/428); and a boot whose
# shape is not stated is a boot whose numbers cannot be read (docs/452). So this script is the only
# supported way to flip it, and it prints the resulting state instead of assuming it.
#
#   scripts/pocket/logswitch.sh on     -- LOG=1, and it warns that the card is no longer a player card
#   scripts/pocket/logswitch.sh off    -- LOG=0, the shipped shape
#   scripts/pocket/logswitch.sh        -- report only
set -e
M="${CARD_MOUNT:-/Volumes/Pocket}"
INI="$M/Assets/night/common/night.ini"
[ -f "$INI" ] || { echo "$INI not found -- is the card mounted?"; exit 1; }

case "${1:-show}" in
    on)  want=1 ;;
    off) want=0 ;;
    show) want="" ;;
    *) echo "usage: $0 [on|off|show]"; exit 1 ;;
esac

if [ -n "$want" ]; then
    python3 - "$INI" "$want" <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1]); t = p.read_text()
t2 = re.sub(r'(?m)^LOG=.*$', 'LOG=%s' % sys.argv[2], t)
assert 'LOG=' in t2, 'night.ini has no LOG= key -- deployed from a tree that predates it?'
p.write_text(t2)
PY
    sync
fi

now=$(grep -E '^LOG=' "$INI" | tr -d ' ')
if [ "$now" = "LOG=1" ]; then
    echo "LOG=1  -- DIAGNOSTIC CARD: the app writes Saves/<app>/common/nightlog.sav every 32 lines."
    echo "         Play feel measured on this card is not player feel (docs/452)."
    echo "         After the boot: scripts/pocket/logsave.sh   (before the next boot overwrites it)"
elif [ "$now" = "LOG=0" ]; then
    echo "LOG=0  -- shipped player shape: no file is written, and nothing on the card can tell us"
    echo "         what a boot did. That is the correct default; switch it on deliberately."
else
    echo "LOG= unreadable in $INI"
    exit 1
fi

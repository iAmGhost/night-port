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
#
#   --stall N   -- also set STALL_MS=N, the report-only threshold (`main_of.c:3207`: a frame gap that
#                  passes it testifies once, naming the resource in flight).  The shipped 400 ms catches the
#                  stage-entry stalls docs/433 chased; a hitch you can feel but cannot name is smaller than
#                  that, so a stutter hunt asks 120.  Passing it is not a hand-edit any more, which is the
#                  whole point: hand-edits are reverted by the next deploy (docs/428), a flag is not.
set -e
M="${CARD_MOUNT:-/Volumes/Pocket}"
INI="$M/Assets/night/common/night.ini"
[ -f "$INI" ] || { echo "$INI not found -- is the card mounted?"; exit 1; }

want="" stall=""
while [ $# -gt 0 ]; do
    case "$1" in
        on)    want=1 ;;
        off)   want=0 ;;
        show)  want="" ;;
        --stall) stall="${2:-}"; [ -n "$stall" ] || { echo "--stall needs a number of ms"; exit 1; }; shift;;
        *) echo "usage: $0 [on|off|show] [--stall N]"; exit 1 ;;
    esac
    shift
done

if [ -n "$want" ] || [ -n "$stall" ]; then
    python3 - "$INI" "${want:-}" "${stall:-}" <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1]); t = p.read_text()
want, stall = sys.argv[2], sys.argv[3]
if want:
    t2 = re.sub(r'(?m)^LOG=.*$', 'LOG=%s' % want, t)
    assert 'LOG=' in t2, 'night.ini has no LOG= key -- deployed from a tree that predates it?'
    t = t2
if stall:
    assert re.search(r'(?m)^STALL_MS=', t), 'night.ini has no STALL_MS= key'
    t = re.sub(r'(?m)^STALL_MS=.*$', 'STALL_MS=%s' % stall, t)
p.write_text(t)
PY
    sync
fi

# State is printed as a shape, never inferred (docs/452: a boot whose shape is not stated is a boot whose
# numbers cannot be read).  Everything the switch touched, plus DIAG, which decides what else is on.
now=$(grep -E '^LOG=' "$INI" | tr -d ' ')
echo "shape: $(grep -E '^(LOG|DIAG|STALL_MS|TICK_HZ|DISC_BATCH|DISC_LOOKAHEAD|DELAY_US|PRESENT_CAP_MS|PRESENT_TESTLESS|PRESENT_SKIP|FB_MODE)=' "$INI" | tr '\n' ' ')"
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

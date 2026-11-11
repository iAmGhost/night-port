#!/bin/sh
# Turn the card log on or off -- and say what the card now says it is.
#
# WHY A SCRIPT AND NOT AN EDIT.  `LOG=` is the one ini key whose value decides whether a boot is
# observable at all, and two failures of this shape are on the record: deploy.sh rewrites night.ini
# from the dist tree, so a hand-set knob is silently reverted under you (docs/428); and a boot whose
# shape is not stated is a boot whose numbers cannot be read (docs/452). So this script is the only
# supported way to flip it, and it prints the resulting state instead of assuming it.
#
#   scripts/pocket/logswitch.sh on      -- LOG=1, and it warns that the card is no longer a player card
#   scripts/pocket/logswitch.sh summary -- LOG=2, the shipped shape: the run's shape (~6 lines a report),
#                                        not its minutes. A player boot that can still be argued with
#                                        (docs/456 13); branchfps.py reads a summary log
#   scripts/pocket/logswitch.sh off    -- LOG=0, the shipped shape
#   scripts/pocket/logswitch.sh        -- report only
#
#   --stall N   -- also set STALL_MS=N, the report-only threshold (`main_of.c:3207`: a frame gap that
#                  passes it testifies once, naming the resource in flight).  The shipped 400 ms catches the
#                  stage-entry stalls docs/433 chased; a hitch you can feel but cannot name is smaller than
#                  that, so a stutter hunt asks 120.  Passing it is not a hand-edit any more, which is the
#                  whole point: hand-edits are reverted by the next deploy (docs/428), a flag is not.
#
#   --set K=V -- set ANY ini key, verified by re-reading the file. The point of this script is that no
#                  knob reaches a card by hand any more (docs/428: hand-edits are reverted by the next
#                  deploy, a flag is not), so the flag has to cover the knobs it is used with.
#   --music off -- CDDA_OFF=1, and --music on restores 0.  A slide fade costs ~25 ms a copy and the CD-DA
#                  feed spends 30-110% of the same window (docs/456 9): the two candidates for "the screen
#                  pauses" are separated by one boot with the music off, and only a flag can be trusted not
#                  to be reverted (docs/428).
set -e
M="${CARD_MOUNT:-/Volumes/Pocket}"
INI="$M/Assets/night/common/night.ini"
[ -f "$INI" ] || { echo "$INI not found -- is the card mounted?"; exit 1; }

want="" stall="" music="" sets=""
while [ $# -gt 0 ]; do
    case "$1" in
        on)    want=1 ;;
        off)   want=0 ;;
        summary) want=2 ;;   # the shipped shape: run shape only (docs/456 13)
        ram)     want=3 ;;   # full detail to an SDRAM ring, ONE card append per LOG_FLUSH_MS (docs/456 13)
        null)    want=4 ;;   # format every line and THROW IT AWAY: no open, no handle, no bytes (docs/456 13)
        show)  want="" ;;
        --stall) stall="${2:-}"; [ -n "$stall" ] || { echo "--stall needs a number of ms"; exit 1; }; shift;;
        --set)   sets="$sets ${2:-}"; case "${2:-}" in [A-Z]*=[0-9]*) ;; *) echo "--set wants KEY=VALUE"; exit 1;; esac; shift;;
        --music) music="${2:-}"; case "$music" in off) music=1;; on) music=0;; *) echo "--music wants on|off"; exit 1;; esac; shift;;
        *) echo "usage: $0 [on|summary|ram|null|off|show] [--stall N] [--music on|off]"; exit 1 ;;
    esac
    shift
done

if [ -n "$want" ] || [ -n "$stall" ] || [ -n "$music" ] || [ -n "$sets" ]; then
    python3 - "$INI" "${want:-}" "${stall:-}" "${music:-}" ${sets:+$sets} <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1]); t = p.read_text()
want, stall, music = sys.argv[2], sys.argv[3], sys.argv[4]
for kv in sys.argv[5:]:
    k, v = kv.split("=", 1)
    assert re.search(r'(?m)^%s=' % k, t), 'night.ini has no %s= key (deployed from an older tree?)' % k
    t = re.sub(r'(?m)^%s=.*$' % k, '%s=%s' % (k, v), t)
if want:
    t2 = re.sub(r'(?m)^LOG=.*$', 'LOG=%s' % want, t)
    assert 'LOG=' in t2, 'night.ini has no LOG= key -- deployed from a tree that predates it?'
    t = t2
if stall:
    assert re.search(r'(?m)^STALL_MS=', t), 'night.ini has no STALL_MS= key'
    t = re.sub(r'(?m)^STALL_MS=.*$', 'STALL_MS=%s' % stall, t)
if music:
    assert re.search(r'(?m)^CDDA_OFF=', t), 'night.ini has no CDDA_OFF= key'
    t = re.sub(r'(?m)^CDDA_OFF=.*$', 'CDDA_OFF=%s' % music, t)
p.write_text(t)
PY
    sync
fi

# State is printed as a shape, never inferred (docs/452: a boot whose shape is not stated is a boot whose
# numbers cannot be read).  Everything the switch touched, plus DIAG, which decides what else is on.
now=$(grep -E '^LOG=' "$INI" | tr -d ' ')
echo "shape: $(grep -E '^(LOG|DIAG|STALL_MS|TICK_HZ|DISC_BATCH|DISC_LOOKAHEAD|DELAY_US|PRESENT_CAP_MS|PRESENT_TESTLESS|PRESENT_SKIP|FB_MODE|CDDA_OFF|PING|PING_MS)=' "$INI" | tr '\n' ' ')"
# Verify rather than trust: the shape printed must be the shape on the card (docs/452).
grep -qE "^PING(=[0-9]*)?$" "$INI" || { echo "PING= unreadable in $INI"; exit 1; }
if [ "$now" = "LOG=2" ]; then
    echo "LOG=2  -- SHAPE LOG: the run's shape (~6 lines a report: settings, blit shape, perf,"
    echo "         split wall, gap worst, stages last, view=) into Saves/<app>/common/nightlog.sav."
    echo "         A player boot that can still be argued with, at a fifth of the SD traffic (docs/456 13)."
    echo "         After the boot: scripts/pocket/logsave.sh, then scripts/pocket/branchfps.py"
elif [ "$now" = "LOG=1" ]; then
    echo "LOG=1  -- DIAGNOSTIC CARD: the app writes Saves/<app>/common/nightlog.sav every 32 lines."
    echo "         Play feel measured on this card is not player feel (docs/452)."
    echo "         After the boot: scripts/pocket/logsave.sh   (before the next boot overwrites it)"
elif [ "$now" = "LOG=3" ]; then
    echo "LOG=3  -- RAM RING: every line formatted into 256 KB of SDRAM, pushed to the card as ONE"
    echo "         append every LOG_FLUSH_MS (default 30 000).  The report block still runs every second,"
    echo "         so this is the arm that asks whether the smoothing was the CARD EVENT or the WORK"
    echo "         (docs/456 13).  A power-off loses the ring's last unflushed <=LOG_FLUSH_MS; a normal"
    echo "         exit flushes.  Read the boot with branchfps.py -- it prints the appends-per-report."
elif [ "$now" = "LOG=4" ]; then
    echo "LOG=4  -- NULL SINK: the report block formats every line as at LOG=1 and DISCARDS it. Nothing is"
    echo "         opened, no slot handle is held, the card sees nothing. This is the arm that asks whether"
    echo "         the smoothing was the WORK (smooth -> yes) or the OPEN HANDLE (stutters -> yes, and the"
    echo "         mechanism is in the OS's save-slot handling, not in anything we write) (docs/456 13)."
    echo "         THERE IS NO LOG BY DESIGN: prove the boot by the PREVIOUS log surviving byte-identical"
    echo "         (md5 -q Saves/night/common/nightlog.sav must NOT change), plus the player's own words."
elif [ "$now" = "LOG=0" ]; then
    echo "LOG=0  -- shipped player shape: no file is written, and nothing on the card can tell us"
    echo "         what a boot did. That is the correct default; switch it on deliberately."
else
    echo "LOG= unreadable in $INI"
    exit 1
fi

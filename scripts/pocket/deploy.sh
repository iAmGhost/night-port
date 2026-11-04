#!/usr/bin/env bash
# Deploy the assembled NIGHT core AND its data slots to a Pocket SD card.
#
# The SDK's own platforms/pocket/copy.sh copies only the flat core tree it assembled (Cores/, Assets/,
# Platforms/) -- image.sh has no notion of the ~600 MB of game data, which lives in the regenerable
# scratch tree and is far too big to be a build artifact.  This script does both halves in one go:
#
#   1. the core tree            build/pocket/night/            (bitstream, loader, os.bin, elf, ini, JSONs)
#   2. the data-slot files     scratch/pocket/                 (NIGHT.RES + one .bin per CDDA track)
#
# Slot files are FLAT inside Assets/<instance>/common/ -- the same folder image.sh puts night.elf,
# os.bin and bank.ofsf in -- because the OS matches a data slot by file name and extension, not by a
# path (pocket/dist/night/Cores/iamghost.night/data.json is the binding; track names are short enough
# for the Pocket's 23-character file-name limit, "track02.bin" = 11).
#
# Usage: deploy.sh <core-tree> <data-dir> [sd-path]
#        make -C pocket/src/night deploy        (uses build/pocket/night and scratch/pocket)

set -euo pipefail

OUT="${1:?usage: deploy.sh <core-tree> <data-dir> [sd-path]}"
DATA="${2:?usage: deploy.sh <core-tree> <data-dir> [sd-path]}"
SDCARD="${3:-${SD_PATH:-}}"

GREEN='\033[92m'; RED='\033[91m'; RESET='\033[0m'
ok()   { echo -e "  ${GREEN}+${RESET} $1"; }
fail() { echo -e "  ${RED}x${RESET} $1" >&2; exit 1; }

[ -d "$OUT/Cores" ] || fail "no core tree at $OUT (run 'make' in pocket/src/night first)"
[ -d "$DATA" ]      || fail "no data dir at $DATA (run 'make data', i.e. scripts/pocket/repack.py)"

SDK_ROOT="$(cd "$(dirname "$0")/../../pocket" && pwd)"
SDETECT=""
for cand in "$SDK_ROOT/scripts/sdcard.sh" "$SDK_ROOT/src/sdk/scripts/sdcard.sh"; do
    [ -f "$cand" ] && { SDETECT="$cand"; break; }; done
[ -n "$SDETECT" ] || fail "vendored SD detector missing under $SDK_ROOT (scripts/pocket/sync_sdk.sh rebuilds pocket/)"

# ── find the card (the SDK's own detector: /Volumes/* on macOS, /run/media/… on Linux) ──
source "$SDETECT"
[ -d "$SDCARD" ] || fail "SD card not found (pass its mount point as the third argument)"

# ── 1. the core tree ────────────────────────────────────────────────────────────────────
echo "Deploying $OUT to $SDCARD"
mkdir -p "$SDCARD/Cores" "$SDCARD/Assets" "$SDCARD/Platforms"
cp -r "$OUT"/Cores/*     "$SDCARD/Cores/"     && ok "Cores"
cp -r "$OUT"/Assets/*    "$SDCARD/Assets/"    && ok "Assets"
cp -r "$OUT"/Platforms/* "$SDCARD/Platforms/" && ok "Platforms"

# The instance's asset folder is where image.sh put night.elf / os.bin / bank.ofsf; the data slots are
# read from the same folder.  Derive the platform directory from the tree being DEPLOYED -- scanning the
# card for "any Assets/*/common" lands in whichever core was there first (measured: a first version put
# 600 MB of NIGHT data into the card's existing Assets/2600/common).
PLAT=""
for d in "$OUT"/Assets/*; do [ -d "$d/common" ] && { PLAT="$(basename "$d")"; break; }; done
[ -n "$PLAT" ] || fail "no Assets/<platform>/common in $OUT (image.sh produced an incomplete tree)"
COMMON="$SDCARD/Assets/$PLAT/common"
mkdir -p "$COMMON"
[ -f "$COMMON/night.elf" ] || fail "$COMMON has no night.elf -- core tree did not land"

# ── 2. the data-slot files ──────────────────────────────────────────────────────────────
need=0
[ -f "$DATA/NIGHT.RES" ] || fail "missing $DATA/NIGHT.RES -- the game terminates without it (fn_03690b -> fn_036537)"
need=$(( $(stat -f%z "$DATA/NIGHT.RES" 2>/dev/null || stat -c%s "$DATA/NIGHT.RES") ))
for f in "$DATA"/cd/track*.bin; do
    [ -f "$f" ] || continue
    b=$(stat -f%z "$f" 2>/dev/null || stat -c%s "$f"); need=$(( need + b ))
done
free=$(df -k "$SDCARD" | awk 'NR==2 {print $4}')
[ "$free" -gt $(( need / 1024 + 4096 )) ] || fail "card has ${free} KB free, the slot files need $(( need / 1024 )) KB"

cp "$DATA/NIGHT.RES" "$COMMON/NIGHT.RES" && ok "NIGHT.RES -> $(basename "$COMMON")/ (slot 4)"
count=0
for f in "$DATA"/cd/track*.bin; do
    [ -f "$f" ] || continue
    name=$(basename "$f")
    [ ${#name} -le 23 ] || fail "\"$name\" is ${#name} characters; the Pocket allows 23"
    base="${name%.bin}"
    case "$base" in track[0-9][0-9]) : ;; *) fail "\"$name\" is not named trackNN.bin" ;; esac
    slot=$(( 16 + 10#${base#track} ))          # track02 -> slot 18 ... track15 -> slot 31
    cp "$f" "$COMMON/$name" && ok "$name -> $(basename "$COMMON")/ (slot $slot)"
    count=$(( count + 1 ))
done
[ "$count" -gt 0 ] || fail "no track*.bin under $DATA/cd (run scripts/pocket/repack.py)"

# Nonvolatile slots must be VISIBLE before the host can open one: of_log.c and of_save.c find their slot
# through readdir("/") + d_ino, and openfpgaOS-Quake's sys_of.c returns NULL when readdir does not list it.
# A bound-but-absent file is therefore pre-created empty here, and the core writes it.
for pre in night.cfg night_0.sav night_1.sav night_2.sav night_3.sav night_4.sav night_5.sav night_6.sav night_7.sav; do
    [ -f "$COMMON/$pre" ] || : > "$COMMON/$pre"
    ok "pre-created $pre"
done

# The firmware keeps nonvolatile slot data under Saves/<platform>/common/ (measured: Quake's cfg slot is at
# Saves/quake/common/quake.cfg, and mounting this core created Saves/night/common/ on its own).  A slot with
# no file on either side has nothing for the app's fopen to land in, so the same placeholders go there too.
SAVE_DIR="$SDCARD/Saves/$(basename "$PLAT")/common"
mkdir -p "$SAVE_DIR"
for pre in night.cfg night_0.sav night_1.sav night_2.sav night_3.sav night_4.sav night_5.sav night_6.sav night_7.sav; do
    [ -f "$SAVE_DIR/$pre" ] || : > "$SAVE_DIR/$pre"
done
ok "placeholders in Saves/$(basename "$PLAT")/common/"

# Saves and settings are created by the core on the card; a stale manifest copy is not wanted.
sync 2>/dev/null || true
echo -e "${GREEN}Deployed:${RESET} core tree + NIGHT.RES + $count track slots to $SDCARD"
echo "Eject the card and boot the Pocket."

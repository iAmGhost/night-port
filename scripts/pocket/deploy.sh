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

size_of() { stat -f%z "$1" 2>/dev/null || stat -c%s "$1"; }
md5_of()  { md5 -q "$1" 2>/dev/null || md5sum "$1" | cut -d' ' -f1; }

# The slot files are ~600 MB of disc audio that never changes between builds, and copying them every time a
# core JSON or the ELF changes is the slow part of a hardware iteration loop (tens of seconds of USB writes).
# scratch/pocket/deploy_manifest.tsv remembers what the last deploy put on the card: a file whose own md5
# matches the record, and whose twin on the card still has the same size, is skipped.  A card-side deletion
# shows up as a size mismatch and gets recopied.
MANIFEST="$DATA/deploy_manifest.tsv"
[ -f "$MANIFEST" ] || printf '# file\tmd5\tsize\n' > "$MANIFEST"
copy_slot() {                     # copy_slot <src> <dst> <label>
    local src="$1" dst="$2" label="$3" name sz src_md5 rec
    name="$(basename "$dst")"
    sz=$(size_of "$src")
    src_md5=$(md5_of "$src")
    rec=$(awk -F'\t' -v n="$name" '$1==n {print $2" "$3; exit}' "$MANIFEST")
    if [ -f "$dst" ] && [ "$(size_of "$dst")" = "$sz" ] && [ "$rec" = "$src_md5 $sz" ]; then
        ok "$label unchanged -- skipped"
        return 0
    fi
    # No record yet (first run of this version, or a wiped manifest) but the card already holds a file of the
    # same size: verify it once by md5 and adopt it into the manifest.  That read pass happens once ever, and
    # it is what keeps the first re-deploy after a core change from pushing 600 MB that never changed.
    if [ -z "$rec" ] && [ -f "$dst" ] && [ "$(size_of "$dst")" = "$sz" ] && [ "$(md5_of "$dst")" = "$src_md5" ]; then
        grep -v "^$name\\t" "$MANIFEST" > "$MANIFEST.tmp" && mv "$MANIFEST.tmp" "$MANIFEST"
        printf '%s\t%s\t%s\n' "$name" "$src_md5" "$sz" >> "$MANIFEST"
        ok "$label already on the card (md5 verified) -- not recopied"
        return 0
    fi
    cp "$src" "$dst" || fail "copy $src -> $dst failed"
    grep -v "^$name\\t" "$MANIFEST" > "$MANIFEST.tmp" && mv "$MANIFEST.tmp" "$MANIFEST"
    printf '%s\t%s\t%s\n' "$name" "$src_md5" "$sz" >> "$MANIFEST"
    ok "$label copied"
}

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
# COPYFILE_DISABLE: macOS cp otherwise writes AppleDouble sidecars (._night.elf, ._NIGHT.json, ._data.json)
# next to every copied file.  They carry the extensions the firmware scans for -- a ._NIGHT.json in an instance
# folder is exactly the kind of stray entry that makes a data-slot scan interesting to explain.
export COPYFILE_DISABLE=1

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

# Which APF file id each file is bound to comes from the manifest generator, never from arithmetic here: ids
# 10-19 are the OS save window, and a data file bound to one of them is served 256 KB only (README:644).
# A lookup rather than an associative array: /bin/bash on macOS is 3.2 and has no `declare -A`.
slot_id_of() {
    [ -f "$OUT/slot_map.tsv" ] || { printf '?'; return 0; }
    awk -F'\t' -v n="$1" '$2==n {print $1; found=1; exit} END {if (!found) printf "?"}' "$OUT/slot_map.tsv"
}
[ -f "$COMMON/night.elf" ] || fail "$COMMON has no night.elf -- core tree did not land"

# Drop this core's own persisted interact values.  They were written by our earlier builds, whose two sliders
# behaved badly: the card's Settings/iamghost.night/Interact/_core/interact_persist.json carried Music Volume 0
# and Tick Pump 60 (a 0 the UI clamps to the slider's minimum), and 60 is a legal tick cap -- 8x slow motion.
# 0.1.25 ships both knobs as named lists, so they start from the values the core actually agrees to.  The
# firmware regenerates this file; the owner's input remap (Input/_core/) is left alone.
if [ -d "$SDCARD/Settings/iamghost.night/Interact" ]; then
    rm -rf "$SDCARD/Settings/iamghost.night/Interact"
    ok "cleared this core's stale persisted interact values (they came from the 0.1.24 sliders)"
fi

# ── 2. the data-slot files ──────────────────────────────────────────────────────────────
need=0
[ -f "$DATA/NIGHT.RES" ] || fail "missing $DATA/NIGHT.RES -- the game terminates without it (fn_03690b -> fn_036537)"
need=$(( $(size_of "$DATA/NIGHT.RES") ))
for f in "$DATA"/cd/track*.bin; do
    [ -f "$f" ] || continue
    need=$(( need + $(size_of "$f") ))
done
free=$(df -k "$SDCARD" | awk 'NR==2 {print $4}')
[ "$free" -gt $(( need / 1024 + 4096 )) ] || fail "card has ${free} KB free, the slot files need $(( need / 1024 )) KB"

res_slot=$(slot_id_of "NIGHT.RES")
copy_slot "$DATA/NIGHT.RES" "$COMMON/NIGHT.RES" "NIGHT.RES -> $(basename "$COMMON")/ (slot $res_slot)"
count=0
skipped=0
for f in "$DATA"/cd/track*.bin; do
    [ -f "$f" ] || continue
    name=$(basename "$f")
    [ ${#name} -le 23 ] || fail "\"$name\" is ${#name} characters; the Pocket allows 23"
    base="${name%.bin}"
    case "$base" in track[0-9][0-9]) : ;; *) fail "\"$name\" is not named trackNN.bin" ;; esac
    slot=$(slot_id_of "$name")                # from dist/night/slot_map.tsv: 10-19 are save slots and must
                                              # not carry audio (README:644)
    label="$name -> $(basename "$COMMON")/ (slot $slot)"
    copy_slot "$f" "$COMMON/$name" "$label"
    count=$(( count + 1 ))
done
[ "$count" -gt 0 ] || fail "no track*.bin under $DATA/cd (run scripts/pocket/repack.py)"

# Nothing is pre-created any more.  The placeholder theory was "a bound-but-absent file has nothing for fopen
# to land in", but the card says otherwise: Quake's Saves/quake/common/ contains exactly two files -- quake.cfg
# (20 B) and s0.sav (35 290 B) -- both created by the core itself at 01:47 today, and openfpgaOS-Quake pre-
# creates nothing.  Worse, empty placeholders made macOS write AppleDouble sidecars (._night_0.sav, 4 KB each)
# next to every one, which is noise on a card the firmware has to read.
SAVE_DIR="$SDCARD/Saves/$(basename "$PLAT")/common"
mkdir -p "$SAVE_DIR"
ok "Saves/$(basename "$PLAT")/common/ exists for the core to write into"

# Strip any AppleDouble sidecars left by older deploys.
for d in "$SDCARD/Cores/iamghost.night" "$SDCARD/Assets/night" "$SDCARD/Assets/night/common" "$SDCARD/Assets/night/iamghost.night" "$SDCARD/Saves/night"; do
    [ -d "$d" ] && find "$d" -name "._*" -maxdepth 1 -delete 2>/dev/null
done
ok "no ._ sidecars in the deployed tree"

# Saves and settings are created by the core on the card; a stale manifest copy is not wanted.
sync 2>/dev/null || true
echo -e "${GREEN}Deployed:${RESET} core tree + $count slot files ($(wc -l < "$MANIFEST") tracked) to $SDCARD"
echo "Eject the card and boot the Pocket."

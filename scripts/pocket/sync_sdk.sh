#!/usr/bin/env bash
# Vendor the openfpgaSDK (openfpgaOS app SDK) into pocket/ so the Pocket build
# has its sdk.mk, headers, bundled musl, APF platform scripts and prebuilt
# runtime (bitstream + os.bin + loader) locally.
#
#   bash scripts/pocket/sync_sdk.sh            # from ~/Git/openfpgaSDK
#   OPENFPGA_SDK=/path/to/sdk bash scripts/pocket/sync_sdk.sh
#
# What lands in pocket/:  src/sdk/, runtime/, scripts/, tools/, dist/sdk/,
# Makefile, LICENSES/, SDK-README.md, SDK-GETTING_STARTED.md.  All of it is
# git-ignored (pocket/.gitignore) — the port's own sources are pocket/src/night/
# and pocket/dist/night/.
#
# The vendored tree is copied, not symlinked: image.sh and copy.sh resolve
# ROOT/runtime paths relative to the app directory, and a symlink would make
# the assembled SD tree depend on where the SDK happens to live.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="${OPENFPGA_SDK:-$HOME/Git/openfpgaSDK}"
DEST="$REPO/pocket"

[ -d "$SRC/src/sdk" ] || { echo "no openfpgaSDK at $SRC (set OPENFPGA_SDK=...)"; exit 1; }
mkdir -p "$DEST/src"

rsync -a --delete \
  --exclude '.git' --exclude 'build' --exclude '.obj' --exclude 'releases' \
  --exclude 'src/apps' --exclude 'src/mygame' \
  "$SRC/src/sdk/" "$DEST/src/sdk/"
rsync -a --delete "$SRC/runtime/" "$DEST/runtime/"
rsync -a --delete "$SRC/scripts/" "$DEST/scripts/"
rsync -a --delete "$SRC/tools/"  "$DEST/tools/"
rsync -a --delete "$SRC/dist/sdk/" "$DEST/dist/sdk/"
rsync -a "$SRC/LICENSE" "$SRC/NOTICE" "$SRC/LICENSES/" "$SRC/REUSE.toml" "$DEST/"
# The SDK's own docs are reference material here; pocket/README.md is ours.
cp "$SRC/README.md"         "$DEST/SDK-README.md"
cp "$SRC/GETTING_STARTED.md" "$DEST/SDK-GETTING_STARTED.md"
cp "$SRC/Makefile"          "$DEST/Makefile"
rm -f "$DEST/AGENTS.md" "$DEST/CLAUDE.md" "$DEST/.DS_Store" "$DEST/runtime/.DS_Store"

echo "vendored openfpgaSDK ($SRC) -> $DEST"
echo "toolchain check:"
command -v riscv64-elf-gcc >/dev/null || echo "  riscv64-elf-gcc NOT found (brew install riscv64-elf-gcc)"
riscv64-elf-gcc --version 2>/dev/null | head -1 | sed 's/^/  /'

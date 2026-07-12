#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ARCHIVE="${TMPDIR:-/tmp}/vionature-butler.zip"
INSTALL_DIR="$ROOT/tools/butler"
URL="${BUTLER_DOWNLOAD_URL:-https://broth.itch.zone/butler/linux-amd64/LATEST/archive/default}"

echo "========================================"
echo "  Installing itch.io butler"
echo "========================================"
echo "URL: $URL"
echo "Install dir: $INSTALL_DIR"

mkdir -p "$INSTALL_DIR"
curl -L "$URL" -o "$ARCHIVE"
unzip -o "$ARCHIVE" -d "$INSTALL_DIR"
chmod +x "$INSTALL_DIR/butler"

echo ""
"$INSTALL_DIR/butler" -V
echo ""
echo "Done. publish-itch.sh will use this local butler by default."

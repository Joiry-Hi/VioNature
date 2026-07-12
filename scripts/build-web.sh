#!/bin/bash
# Build VioNature for Web (HTML5/WebAssembly)
set -euo pipefail
cd "$(dirname "$0")/.."

ROOT="$(pwd)"
RELEASE_DIR=web-release
BUILD_DIR="$RELEASE_DIR/build"
DIST_DIR="$RELEASE_DIR/dist"
PACKAGE_DIR="$RELEASE_DIR/packages"
ZIP_NAME="$PACKAGE_DIR/vionature_web.zip"
ITCH_ZIP_NAME="$PACKAGE_DIR/vionature_itch_html5.zip"
TOOLCHAIN=/usr/share/emscripten/cmake/Modules/Platform/Emscripten.cmake

mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"
mkdir -p "$PACKAGE_DIR"

emcmake cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=Web \
    -DJPH_USE_DX12=OFF \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"

# Emscripten's preload archive is produced at link time. CMake does not always
# notice changes in config/ or assets/, so force relink to keep vionature.data
# in sync even when only gameplay.cfg, BGM, SFX, or other assets changed.
rm -f "$BUILD_DIR/vionature.js" "$BUILD_DIR/vionature.wasm" "$BUILD_DIR/vionature.data"

cmake --build "$BUILD_DIR" --target MyShooter -j "$(nproc)"

rm -rf "$DIST_DIR" "$ZIP_NAME" "$ITCH_ZIP_NAME"
mkdir -p "$DIST_DIR"

cp "$BUILD_DIR/vionature.js" "$DIST_DIR/vionature.js"
cp "$BUILD_DIR/vionature.wasm" "$DIST_DIR/vionature.wasm"
cp "$BUILD_DIR/vionature.data" "$DIST_DIR/vionature.data"
export BUILD_DIR DIST_DIR
python3 - <<'PY'
import os
from pathlib import Path

shell = Path("web/shell.html").read_text()
html = shell.replace("{{{ SCRIPT }}}", '<script src="vionature.js"></script>')
dist_dir = Path(os.environ["DIST_DIR"])
build_dir = Path(os.environ["BUILD_DIR"])
(dist_dir / "index.html").write_text(html)
(dist_dir / "vionature.html").write_text(html)
(build_dir / "index.html").write_text(html)
(build_dir / "vionature.html").write_text(html)
PY

(cd "$DIST_DIR" && zip -qr "$ROOT/$ZIP_NAME" index.html vionature.html vionature.js vionature.wasm vionature.data)
(cd "$DIST_DIR" && zip -qr "$ROOT/$ITCH_ZIP_NAME" index.html vionature.html vionature.js vionature.wasm vionature.data)

echo ""
echo "==> Web build ready:"
ls -lh "$DIST_DIR"/index.html "$DIST_DIR"/vionature.html "$DIST_DIR"/vionature.js "$DIST_DIR"/vionature.wasm "$DIST_DIR"/vionature.data "$ZIP_NAME" "$ITCH_ZIP_NAME"
echo ""
echo "Serve: cd $DIST_DIR && python3 -m http.server 8080"
echo "Then open: http://localhost:8080/"
echo "Upload to itch.io: $ITCH_ZIP_NAME"

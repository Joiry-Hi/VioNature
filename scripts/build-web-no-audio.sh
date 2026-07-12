#!/bin/bash
# Build a lightweight WebAssembly release package without audio assets.
# Intended for bandwidth-sensitive demo hosts such as wakudemo.
set -euo pipefail
cd "$(dirname "$0")/.."

ROOT="$(pwd)"
RELEASE_DIR=web-release-no-audio
BUILD_DIR="$RELEASE_DIR/build"
DIST_DIR="$RELEASE_DIR/dist"
PACKAGE_DIR="$RELEASE_DIR/packages"
STAGING_DIR="$RELEASE_DIR/staging"
STAGING_CONFIG="$STAGING_DIR/config"
STAGING_ASSETS="$STAGING_DIR/assets"
ZIP_NAME="$PACKAGE_DIR/vionature_web_no_audio.zip"
WAKUDEMO_ZIP_NAME="$PACKAGE_DIR/vionature_wakudemo_no_audio.zip"
TOOLCHAIN=/usr/share/emscripten/cmake/Modules/Platform/Emscripten.cmake

AUDIO_PATTERNS=(
    "*.wav" "*.mp3" "*.ogg" "*.flac" "*.m4a" "*.aac"
    "*.xm" "*.mod" "*.it" "*.s3m"
)

mkdir -p "$BUILD_DIR" "$PACKAGE_DIR" "$STAGING_DIR"
rm -rf "$STAGING_CONFIG" "$STAGING_ASSETS"
mkdir -p "$STAGING_CONFIG" "$STAGING_ASSETS"

cp -r "$ROOT/config/." "$STAGING_CONFIG/"
cp -r "$ROOT/assets/." "$STAGING_ASSETS/"

before_count=$(find "$STAGING_ASSETS" -type f | wc -l)
for pattern in "${AUDIO_PATTERNS[@]}"; do
    find "$STAGING_ASSETS" -type f -iname "$pattern" -delete
done
find "$STAGING_ASSETS" -type d -empty -delete || true
after_count=$(find "$STAGING_ASSETS" -type f | wc -l)
removed_count=$((before_count - after_count))

emcmake cmake -S . -B "$BUILD_DIR"     -DCMAKE_BUILD_TYPE=Release     -DPLATFORM=Web     -DJPH_USE_DX12=OFF     -DVIONATURE_NO_AUDIO=ON     -DVIONATURE_WEB_CONFIG_DIR="$STAGING_CONFIG"     -DVIONATURE_WEB_ASSETS_DIR="$STAGING_ASSETS"     -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"

# Force relink so vionature.data always reflects the staged no-audio assets.
rm -f "$BUILD_DIR/vionature.js" "$BUILD_DIR/vionature.wasm" "$BUILD_DIR/vionature.data"

cmake --build "$BUILD_DIR" --target MyShooter -j "$(nproc)"

rm -rf "$DIST_DIR" "$ZIP_NAME" "$WAKUDEMO_ZIP_NAME"
mkdir -p "$DIST_DIR"

cp "$BUILD_DIR/vionature.js" "$DIST_DIR/vionature.js"
cp "$BUILD_DIR/vionature.wasm" "$DIST_DIR/vionature.wasm"
cp "$BUILD_DIR/vionature.data" "$DIST_DIR/vionature.data"
export BUILD_DIR DIST_DIR
python3 - <<'PYHTML'
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
PYHTML

(cd "$DIST_DIR" && zip -qr "$ROOT/$ZIP_NAME" index.html vionature.html vionature.js vionature.wasm vionature.data)
(cd "$DIST_DIR" && zip -qr "$ROOT/$WAKUDEMO_ZIP_NAME" index.html vionature.html vionature.js vionature.wasm vionature.data)

echo ""
echo "==> No-audio web build ready:"
echo "    Audio files removed from staged assets: $removed_count"
ls -lh "$DIST_DIR"/index.html "$DIST_DIR"/vionature.html "$DIST_DIR"/vionature.js "$DIST_DIR"/vionature.wasm "$DIST_DIR"/vionature.data "$ZIP_NAME" "$WAKUDEMO_ZIP_NAME"
echo ""
echo "Serve: cd $DIST_DIR && python3 -m http.server 8080"
echo "Then open: http://localhost:8080/"
echo "Upload to wakudemo: $WAKUDEMO_ZIP_NAME"

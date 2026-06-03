#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build-sandbox"
OUTPUT_DIR="$BUILD_DIR/release-linux-en"
ZIP_NAME="VioNature_Linux_EN.zip"

echo "========================================"
echo "  VioNature Linux Release Builder (EN)"
echo "========================================"

# Step 1: Build
echo ""
echo "[1/4] Building Linux release..."
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release -j "$(nproc)"

# Step 2: Collect
echo ""
echo "[2/4] Collecting release files..."
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR/config"
mkdir -p "$OUTPUT_DIR/assets"

# Executables
cp "$BUILD_DIR/MyShooter" "$OUTPUT_DIR/"
cp "$BUILD_DIR/ModelViewer" "$OUTPUT_DIR/"
echo "  -> MyShooter, ModelViewer"

# Assets
echo "  Copying assets..."
cp -r "$ROOT/assets/." "$OUTPUT_DIR/assets/"
rm -f "$OUTPUT_DIR/assets/models/weapons/mystic_staff_v3.mtl"

# Config (English)
echo "  Copying config (English)..."
cp "$ROOT/config/gameplay.cfg" "$OUTPUT_DIR/config/gameplay.cfg"
sed -i 's/^tutorial_language = .*/tutorial_language = english/' "$OUTPUT_DIR/config/gameplay.cfg"

# Docs
echo "  Copying docs..."
cp "$ROOT/GAMEPLAY_GUIDE_EN.md" "$OUTPUT_DIR/GAMEPLAY_GUIDE.md"
cp "$ROOT/README.md" "$OUTPUT_DIR/"

# Step 3: Tar
echo ""
echo "[3/4] Creating tar.gz..."
cd "$OUTPUT_DIR/.."
rm -f "$ZIP_NAME.tar.gz" "$ZIP_NAME"
tar czf "$ZIP_NAME.tar.gz" "$(basename "$OUTPUT_DIR")"
cd "$ROOT"

echo ""
echo "========================================"
echo "  Done!"
echo "  Linux English release: $BUILD_DIR/$ZIP_NAME.tar.gz"
echo "========================================"
ls -lh "$BUILD_DIR/$ZIP_NAME.tar.gz"
echo ""
echo "Release folder: $OUTPUT_DIR"
echo "Files:"
find "$OUTPUT_DIR" -type f | sed "s|$OUTPUT_DIR/||" | sort | head -30
echo "  ..."

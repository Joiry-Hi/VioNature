#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build-windows"
OUTPUT_DIR="$BUILD_DIR/release"
ZIP_NAME="VioNature_Release.zip"
TOOLCHAIN="$ROOT/cmake/toolchain-mingw64.cmake"

MINGW_SYSROOT="/usr/x86_64-w64-mingw32/lib"
GCC_VER_DIR=$(ls -d /usr/lib/gcc/x86_64-w64-mingw32/*-win32 2>/dev/null | head -1)

echo "========================================"
echo "  VioNature Windows Release Builder"
echo "========================================"

# Step 1: Configure
echo ""
echo "[1/4] Configuring CMake for Windows..."
mkdir -p "$BUILD_DIR"
cmake -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DCMAKE_BUILD_TYPE=Release \
    -DJPH_USE_DX12=OFF

# Step 2: Build
echo ""
echo "[2/4] Building..."
cmake --build "$BUILD_DIR" --config Release -j "$(nproc)"

# Step 3: Collect
echo ""
echo "[3/4] Collecting release files..."
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR/config"
mkdir -p "$OUTPUT_DIR/assets/models/weapons"

# Executable
cp "$BUILD_DIR/MyShooter.exe" "$OUTPUT_DIR/"
echo "  -> MyShooter.exe"

# MinGW runtime DLLs
echo "  Copying MinGW runtime DLLs..."
copy_dll() {
    local dll="$1"
    for dir in "$MINGW_SYSROOT" "$GCC_VER_DIR" /usr/x86_64-w64-mingw32/bin; do
        if [ -f "$dir/$dll" ]; then
            cp "$dir/$dll" "$OUTPUT_DIR/"
            echo "  -> $dll"
            return 0
        fi
    done
    # Fallback: search
    local found=$(find /usr/x86_64-w64-mingw32 "$GCC_VER_DIR" -name "$dll" 2>/dev/null | head -1)
    if [ -n "$found" ]; then
        cp "$found" "$OUTPUT_DIR/"
        echo "  -> $dll"
        return 0
    fi
    echo "  WARNING: $dll not found"
}

copy_dll libgcc_s_seh-1.dll
copy_dll libstdc++-6.dll
copy_dll libwinpthread-1.dll

# Assets
echo "  Copying assets..."
cp -r "$ROOT/assets/." "$OUTPUT_DIR/assets/"

# Config
echo "  Copying config..."
cp "$ROOT/config/gameplay.cfg" "$OUTPUT_DIR/config/"

# Step 4: Zip
echo ""
echo "[4/4] Creating zip..."
cd "$OUTPUT_DIR"
rm -f "../$ZIP_NAME"
zip -r "../$ZIP_NAME" ./*
cd "$ROOT"

echo ""
echo "========================================"
echo "  Done!"
echo "  Release package: $BUILD_DIR/$ZIP_NAME"
echo "========================================"
ls -lh "$BUILD_DIR/$ZIP_NAME"
echo ""
echo "Release folder: $OUTPUT_DIR"
echo "Files:"
find "$OUTPUT_DIR" -type f -ls | awk '{print "  " $7, $NF}' | sed "s|$OUTPUT_DIR/||"

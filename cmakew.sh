#!/bin/bash

set -e

usage() {
    echo "Usage: cmakep [-c] -d <target>    # Clean & debug build"
    echo "       cmakep [-c] -r <target>    # Clean & release build"
    exit 1
}

# Defaults
CLEAN=false
MODE=""
TARGET=""

# Parse args (order-independent)
while [[ $# -gt 0 ]]; do
    case "$1" in
    -c)
        CLEAN=true
        shift
        ;;
    -d)
        MODE="Debug"
        shift
        ;;
    -r)
        MODE="Release"
        shift
        ;;
    *)
        if [[ -z "$TARGET" ]]; then
            TARGET="$1"
            shift
        else
            usage
        fi
        ;;
    esac
done

# Validate
if [[ -z "$MODE" || -z "$TARGET" ]]; then
    usage
fi

PRESET="${TARGET}_${MODE}_Windows"
BUILD_DIR="build/${TARGET}_${MODE}"

echo "▶️ Using preset: $PRESET"
echo "📁 Build directory: $BUILD_DIR"

# Clean if requested
if $CLEAN && [[ -d "$BUILD_DIR" ]]; then
    echo "🧹 Cleaning build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# Run CMake preset
echo "⚙️  Running: cmake --preset $PRESET"
cmake --preset "$PRESET"

# Symlink compile_commands.json
if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
    ln -sf "$BUILD_DIR/compile_commands.json" ./compile_commands.json
    echo "✅ Linked compile_commands.json from $BUILD_DIR"
else
    echo "⚠️  No compile_commands.json found in $BUILD_DIR"
fi

# create/update the project-local .clangd file
rm -f .clangd
cat >.clangd <<EOF
CompileFlags:
  CompilationDatabase: $BUILD_DIR
EOF

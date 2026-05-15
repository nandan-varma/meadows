#!/bin/bash
# Package release artifacts
# Usage: ./scripts/ci/package_release.sh [output_dir]
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_DIR="${1:-$PROJECT_ROOT/release}"

mkdir -p "$OUTPUT_DIR"

# Locate the release binary (Windows vs Unix)
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OS" == "Windows_NT" ]]; then
    BIN="$PROJECT_ROOT/build/bin/Meadows.exe"
    DEST="$OUTPUT_DIR/meadows.exe"
else
    BIN="$PROJECT_ROOT/build/bin/Meadows"
    DEST="$OUTPUT_DIR/meadows"
fi

if [[ ! -f "$BIN" ]]; then
    echo "Error: release binary not found at $BIN"
    exit 1
fi

cp "$BIN" "$DEST"
chmod +x "$DEST"

# Documentation
[[ -f "$PROJECT_ROOT/README.md" ]] && cp "$PROJECT_ROOT/README.md" "$OUTPUT_DIR/"
[[ -f "$PROJECT_ROOT/LICENSE" ]]   && cp "$PROJECT_ROOT/LICENSE"   "$OUTPUT_DIR/"

echo "Packaged release to $OUTPUT_DIR/"
ls -lh "$OUTPUT_DIR/"

#!/bin/bash
# Package release artifacts for CI
# Usage: ./scripts/ci/package_release.sh <output_dir>

set -e

OUTPUT_DIR="$1"
if [[ -z "$OUTPUT_DIR" ]]; then
    OUTPUT_DIR="release"
fi

mkdir -p "$OUTPUT_DIR"

# Copy binaries - handle different platforms
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    # Windows
    cp build/bin/Meadows.exe "$OUTPUT_DIR/"
    cp build/tests/meadows_tests.exe "$OUTPUT_DIR/"
else
    # Unix-like
    cp build/bin/Meadows "$OUTPUT_DIR/"
    cp build/tests/meadows_tests "$OUTPUT_DIR/"
fi

cp README.md "$OUTPUT_DIR/"

echo "Packaged release to $OUTPUT_DIR/"
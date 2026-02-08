#!/bin/bash
# Meadows Compiler Build Script
# Usage: ./build.sh [TARGET] [OPTIONS]
#
# TARGETS:
#   all          Build everything (default)
#   debug        Build debug version
#   release      Build release version
#   tests        Build test executables
#   clean        Clean build artifacts
#
# OPTIONS:
#   -j N         Use N parallel jobs
#   -v           Verbose output
#   --llvm PATH  Specify LLVM directory
#
# Examples:
#   ./build.sh                    # Build everything
#   ./build.sh release            # Build release only
#   ./build.sh tests              # Build tests
#   ./build.sh clean              # Clean artifacts

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Default values
TARGET="all"
JOBS=""
VERBOSE=0
LLVM_DIR=""
CMAKE_ARGS=""

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    all|debug|release|tests|clean)
      TARGET="$1"
      shift
      ;;
    -j)
      JOBS="$2"
      shift 2
      ;;
    -v|--verbose)
      VERBOSE=1
      shift
      ;;
    --llvm)
      LLVM_DIR="$2"
      export LLVM_DIR
      shift 2
      ;;
    *)
      CMAKE_ARGS="$CMAKE_ARGS $1"
      shift
      ;;
  esac
done

# Show usage if requested
if [ "$TARGET" == "help" ] || [ "$TARGET" == "-h" ] || [ "$TARGET" == "--help" ]; then
    sed -n '2,20p' "$0"
    exit 0
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows Compiler Build System${NC}"
echo -e "${BLUE}========================================${NC}"
echo

case $TARGET in
    all)
        echo -e "${YELLOW}Building all targets...${NC}"
        echo
        "$SCRIPT_DIR/scripts/build/build_debug.sh" $CMAKE_ARGS
        echo
        "$SCRIPT_DIR/scripts/build/build_release.sh" $CMAKE_ARGS
        echo
        "$SCRIPT_DIR/scripts/build/build_tests.sh" $CMAKE_ARGS
        echo
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}   All builds complete!${NC}"
        echo -e "${GREEN}========================================${NC}"
        echo
        echo "Binaries:"
        echo "  Debug:   build-debug/bin/Meadows"
        echo "  Release: build-release/bin/Meadows"
        echo "  Tests:   build/tests/meadows_tests"
        ;;
    debug)
        "$SCRIPT_DIR/scripts/build/build_debug.sh" $CMAKE_ARGS
        ;;
    release)
        "$SCRIPT_DIR/scripts/build/build_release.sh" $CMAKE_ARGS
        ;;
    tests)
        "$SCRIPT_DIR/scripts/build/build_tests.sh" $CMAKE_ARGS
        ;;
    clean)
        "$SCRIPT_DIR/scripts/build/build_clean.sh"
        ;;
    *)
        echo -e "${RED}Error: Unknown target '$TARGET'${NC}"
        echo "Run './build.sh --help' for usage"
        exit 1
        ;;
esac

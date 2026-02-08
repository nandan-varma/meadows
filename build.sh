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
            echo -e "${RED}Error: Unknown option '$1'${NC}"
            exit 1
            ;;
    esac
done

# Detect LLVM if not provided
if [[ -z "$LLVM_DIR" ]]; then
    source "$SCRIPT_DIR/scripts/dev/detect_llvm.sh"
    LLVM_DIR=$(get_llvm_dir)
fi

if [[ -z "$LLVM_DIR" ]]; then
    echo -e "${RED}Error: LLVM 17 not found${NC}"
    echo "Install LLVM 17 and try again, or use --llvm to specify location"
    exit 1
fi

export LLVM_DIR

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows Compiler Build System${NC}"
echo -e "${BLUE}========================================${NC}"
echo
echo "LLVM: $LLVM_DIR"
echo

case $TARGET in
    all)
        echo -e "${YELLOW}Building all targets...${NC}"
        "$SCRIPT_DIR/scripts/build/build_debug.sh"
        echo
        "$SCRIPT_DIR/scripts/build/build_release.sh"
        echo
        "$SCRIPT_DIR/scripts/build/build_tests.sh"
        echo
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}   All builds complete!${NC}"
        echo -e "${GREEN}========================================${NC}"
        echo "  Debug:   build-debug/bin/Meadows"
        echo "  Release: build-release/bin/Meadows"
        echo "  Tests:   build/tests/meadows_tests"
        ;;
    debug)
        "$SCRIPT_DIR/scripts/build/build_debug.sh"
        ;;
    release)
        "$SCRIPT_DIR/scripts/build/build_release.sh"
        ;;
    tests)
        "$SCRIPT_DIR/scripts/build/build_tests.sh"
        ;;
    clean)
        rm -rf build build-debug build-release
        echo -e "${GREEN}Build directories cleaned${NC}"
        ;;
    *)
        echo -e "${RED}Error: Unknown target '$TARGET'${NC}"
        echo "Run './build.sh --help' for usage"
        exit 1
        ;;
esac

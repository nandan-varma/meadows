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

# Load LLVM detection helpers
source "$SCRIPT_DIR/scripts/dev/detect_llvm.sh"

# Detect LLVM if not provided
if [[ -z "$LLVM_DIR" ]]; then
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

# Build a cmake target in the given build directory.
# Args: label build_type build_dir cmake_target binary_check
build_variant() {
    local label="$1" build_type="$2" build_dir="$SCRIPT_DIR/$3"
    local cmake_target="$4" binary_check="$5"
    local njobs="${JOBS:-$(get_cmake_jobs)}"

    echo -e "${BLUE}Building ${label}...${NC}"
    mkdir -p "$build_dir"
    (
        cd "$build_dir"
        cmake_flags=(-DCMAKE_BUILD_TYPE="$build_type" -DLLVM_DIR="$LLVM_DIR" -DBUILD_TESTS=ON)
        [[ $VERBOSE -eq 1 ]] && cmake_flags+=(--log-level=VERBOSE)
        cmake .. "${cmake_flags[@]}"
        cmake --build . --parallel "$njobs" --target "$cmake_target"
    )

    if [[ ! -f "$build_dir/$binary_check" ]]; then
        echo -e "${RED}Error: ${label} build failed${NC}"
        exit 1
    fi
    echo -e "${GREEN}${label} complete: ${build_dir}/${binary_check}${NC}"
}

case $TARGET in
    all)
        echo -e "${YELLOW}Building all targets...${NC}"
        build_variant "Debug"   Debug   build-debug  Meadows       bin/Meadows
        echo
        build_variant "Release" Release build-release Meadows      bin/Meadows
        echo
        build_variant "Tests"   Debug   build        meadows_tests tests/meadows_tests
        echo
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}   All builds complete!${NC}"
        echo -e "${GREEN}========================================${NC}"
        echo "  Debug:   build-debug/bin/Meadows"
        echo "  Release: build-release/bin/Meadows"
        echo "  Tests:   build/tests/meadows_tests"
        ;;
    debug)
        build_variant "Debug" Debug build-debug Meadows bin/Meadows
        ;;
    release)
        build_variant "Release" Release build-release Meadows bin/Meadows
        ;;
    tests)
        build_variant "Tests" Debug build meadows_tests tests/meadows_tests
        ;;
    clean)
        echo -e "${YELLOW}Cleaning build artifacts...${NC}"
        for d in build build-debug build-release; do
            [[ -d "$SCRIPT_DIR/$d" ]] && echo "  Removing $d/" && rm -rf "$SCRIPT_DIR/$d"
        done
        find "$SCRIPT_DIR" -name "*.ms.ll" -o -name "*.ms.out" | xargs rm -f 2>/dev/null || true
        [[ -f "$SCRIPT_DIR/compile_commands.json" ]] && rm -f "$SCRIPT_DIR/compile_commands.json"
        echo -e "${GREEN}Clean complete${NC}"
        ;;
    *)
        echo -e "${RED}Error: Unknown target '$TARGET'${NC}"
        echo "Run './build.sh --help' for usage"
        exit 1
        ;;
esac

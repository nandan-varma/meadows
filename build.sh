#!/bin/bash
# Main build script for Meadows Compiler

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

show_usage() {
    echo "Usage: $0 [debug|release|tests|clean|all]"
    echo ""
    echo "Options:"
    echo "  debug    Build debug version"
    echo "  release  Build release version"
    echo "  tests    Build test executables"
    echo "  clean    Clean build artifacts"
    echo "  all      Build everything (default)"
    echo ""
    echo "Examples:"
    echo "  $0           # Build everything"
    echo "  $0 debug     # Debug build only"
    echo "  $0 clean     # Clean and exit"
}

run_build() {
    local script="$1"
    local name="$2"
    local script_path="$PROJECT_ROOT/scripts/build/$script"
    
    if [ -f "$script_path" ]; then
        echo -e "${BLUE}========================================${NC}"
        echo -e "${BLUE}Running $name...${NC}"
        echo -e "${BLUE}========================================${NC}"
        bash "$script_path"
    else
        echo -e "${RED}Error: $script not found${NC}"
        exit 1
    fi
}

case "${1:-all}" in
    debug)
        run_build "build_debug.sh" "Debug Build"
        ;;
    release)
        run_build "build_release.sh" "Release Build"
        ;;
    tests)
        run_build "build_tests.sh" "Test Build"
        ;;
    clean)
        run_build "build_clean.sh" "Clean"
        ;;
    all)
        echo -e "${BLUE}Building Meadows Compiler - All Targets${NC}"
        echo ""
        run_build "build_clean.sh" "Clean"
        echo ""
        run_build "build_debug.sh" "Debug Build"
        echo ""
        run_build "build_release.sh" "Release Build"
        echo ""
        run_build "build_tests.sh" "Test Build"
        echo ""
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}All builds complete!${NC}"
        echo -e "${GREEN}========================================${NC}"
        echo ""
        echo "Binaries:"
        echo "  Debug:   $PROJECT_ROOT/build-debug/bin/Meadows"
        echo "  Release: $PROJECT_ROOT/build-release/bin/Meadows"
        echo "  Tests:   $PROJECT_ROOT/build/tests/meadows_tests"
        ;;
    -h|--help)
        show_usage
        ;;
    *)
        echo -e "${RED}Unknown option: $1${NC}"
        show_usage
        exit 1
        ;;
esac

#!/bin/bash

# Meadows Compiler - Build Script
# This script simplifies building the Meadows compiler in different configurations

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "Meadows Compiler - Build Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  debug      Build debug version (default)"
    echo "  release    Build optimized release version"
    echo "  both       Build both debug and release versions"
    echo "  clean      Clean all build directories"
    echo "  test       Build and run tests"
    echo "  help       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                # Build debug version"
    echo "  $0 release        # Build optimized version"
    echo "  $0 both           # Build both versions"
    echo "  $0 clean          # Clean all builds"
    echo "  $0 test           # Build and test"
}

build_debug() {
    print_status "Building debug version..."
    
    cmake -B build -S .
    cmake --build build --parallel
    
    if [ $? -eq 0 ]; then
        print_success "Debug build completed: ./build/meadows"
    else
        print_error "Debug build failed"
        exit 1
    fi
}

build_release() {
    print_status "Building release version..."
    
    cmake -B build-release -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --parallel
    
    if [ $? -eq 0 ]; then
        print_success "Release build completed: ./build-release/meadows"
        
        # Show size comparison if debug build exists
        if [ -f "build/meadows" ]; then
            print_status "Binary size comparison:"
            ls -lh build/meadows build-release/meadows | awk '{print $5 "\t" $9}'
        fi
    else
        print_error "Release build failed"
        exit 1
    fi
}

clean_builds() {
    print_status "Cleaning build directories..."
    
    rm -rf build build-release
    
    print_success "All build directories cleaned"
}

run_tests() {
    print_status "Building and running tests..."
    
    # Build both versions
    build_debug
    build_release
    
    # Run test script if available
    if [ -f "run_tests.sh" ]; then
        ./run_tests.sh
    else
        print_status "Running basic test with release build..."
        if [ -f "tests/test_comprehensive.mds" ]; then
            ./build-release/meadows tests/test_comprehensive.mds
        else
            ./build-release/meadows
        fi
    fi
}

main() {
    local command="${1:-debug}"
    
    case $command in
        "debug")
            build_debug
            ;;
        "release")
            build_release
            ;;
        "both")
            build_debug
            build_release
            ;;
        "clean")
            clean_builds
            ;;
        "test")
            run_tests
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

main "$@"

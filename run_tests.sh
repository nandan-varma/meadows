#!/bin/bash

# Meadows Compiler - Minimal Test Runner Script
# Most logic is now in the C++ test framework

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Simple build function
build_if_needed() {
    # Check if build is needed using C++ test runner
    if [ -f "build/meadows_test" ]; then
        if ./build/meadows_test --build-check >/dev/null 2>&1; then
            return 0  # Build not needed
        fi
    fi
    
    echo "Building Meadows Compiler..."
    
    [ ! -d "build" ] && mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug .. >/dev/null 2>&1
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) >/dev/null 2>&1
    cd ..
    
    print_success "Build completed"
}

# Show usage
show_usage() {
    echo "Meadows Compiler - Simple Test Runner"
    echo ""
    echo "Usage: $0 [command] [options...]"
    echo ""
    echo "Commands:"
    echo "  test          Run tests (default)"
    echo "  build         Build only"
    echo "  clean         Clean build directory"
    echo "  help          Show this help"
    echo ""
    echo "Test Options (passed to C++ test runner):"
    echo "  -q, --quiet   Quiet mode"
    echo "  -m, --minimal Minimal output"
    echo "  -c <category> Run specific category"
    echo ""
    echo "Examples:"
    echo "  $0                    # Build and run all tests"
    echo "  $0 test -q            # Run tests quietly"
    echo "  $0 test -c \"Lexer\"    # Run only Lexer tests"
}

# Main execution
main() {
    local command="test"
    
    # Parse first argument as command if it's not an option
    if [ $# -gt 0 ] && [[ "$1" != -* ]]; then
        command="$1"
        shift
    fi
    
    case $command in
        "build")
            build_if_needed
            ;;
        "clean")
            [ -d "build" ] && rm -rf build && print_success "Build directory cleaned" || echo "Build directory doesn't exist"
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        "test"|*)
            # Check if we're in the right directory
            if [ ! -f "CMakeLists.txt" ]; then
                print_error "This script must be run from the project root directory"
                exit 1
            fi
            
            # Build if needed
            build_if_needed
            
            # Run tests with all remaining arguments passed to C++ test runner
            if [ -f "build/meadows_test" ]; then
                ./build/meadows_test "$@"
                exit $?
            else
                print_error "Test runner not found after build"
                exit 1
            fi
            ;;
    esac
}

main "$@"

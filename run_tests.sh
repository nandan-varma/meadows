#!/bin/bash

# Meadows Compiler Test Runner Script

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Global test statistics
TOTAL_TESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0

# Function to print colored output
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Build the project
build_project() {
    if [ ! -d "build" ]; then
        mkdir build
    fi
    
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug .. >/dev/null 2>&1
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) >/dev/null 2>&1
    cd ..
}

# Run the comprehensive test suite (both C++ framework and Python files)
run_test_suite() {
    echo "Running C++ Test Framework:"
    
    if [ -f "./build/meadows_test" ]; then
        # Run test framework in minimal mode and capture output
        local cpp_output=$(./build/meadows_test "./tests/" minimal 2>/dev/null || true)
        echo "$cpp_output"
        
        # Parse results from the output
        local framework_total=$(echo "$cpp_output" | grep "Total tests:" | grep -o '[0-9]*' | head -1)
        local framework_passed=$(echo "$cpp_output" | grep "Passed:" | grep -o '[0-9]*' | head -1)
        local framework_failed=$(echo "$cpp_output" | grep "Failed:" | grep -o '[0-9]*' | head -1)
        
        # Default to 0 if parsing failed
        framework_total=${framework_total:-0}
        framework_passed=${framework_passed:-0}
        framework_failed=${framework_failed:-0}
        
        TOTAL_TESTS=$((TOTAL_TESTS + framework_total))
        TOTAL_PASSED=$((TOTAL_PASSED + framework_passed))
        TOTAL_FAILED=$((TOTAL_FAILED + framework_failed))
        
    else
        echo "Test runner not found. Make sure build completed successfully."
        TOTAL_FAILED=$((TOTAL_FAILED + 1))
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    fi
    
    echo
    echo "Running Python File Tests:"
    
    # Test individual Python files
    for test_file in tests/*.mds; do
        if [ -f "$test_file" ]; then
            local filename=$(basename "$test_file")
            echo -n "$filename"
            
            if ./build/meadows -p "$test_file" >/dev/null 2>&1; then
                echo " ✓"
                TOTAL_PASSED=$((TOTAL_PASSED + 1))
            else
                echo " ✗"
                TOTAL_FAILED=$((TOTAL_FAILED + 1))
            fi
            TOTAL_TESTS=$((TOTAL_TESTS + 1))
        fi
    done
}

# Print final summary
print_summary() {
    echo
    echo "=========================================="
    echo "Test Results Summary"
    echo "=========================================="
    
    if [ $TOTAL_TESTS -eq 0 ]; then
        echo "No tests were executed."
        return
    fi
    
    # Use awk for percentage calculation (more portable than bc)
    local success_rate=$(awk "BEGIN {printf \"%.1f\", $TOTAL_PASSED * 100 / $TOTAL_TESTS}")
    
    echo "Total Tests: $TOTAL_TESTS"
    echo "Passed: $TOTAL_PASSED"
    echo "Failed: $TOTAL_FAILED"
    echo "Success Rate: ${success_rate}%"
    
    if [ $TOTAL_FAILED -eq 0 ]; then
        print_color $GREEN "🎉 All tests passed!"
    else
        print_color $RED "❌ Some tests failed."
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 [option]"
    echo
    echo "Options:"
    echo "  build         - Build the project"
    echo "  test          - Run comprehensive test suite"
    echo "  clean         - Clean build directory"
    echo "  help          - Show this help message"
    echo "  (no args)     - Run all tests (default)"
    echo
}

# Clean build directory
clean_build() {
    if [ -d "build" ]; then
        rm -rf build
        print_color $GREEN "✓ Build directory cleaned"
    else
        print_color $YELLOW "⚠ Build directory doesn't exist"
    fi
}

# Main execution
main() {
    local command=${1:-all}
    
    case $command in
        "build")
            echo "Building Meadows Compiler..."
            build_project
            ;;
        "test")
            echo "Building Meadows Compiler..."
            build_project
            echo "✓ Build completed"
            echo
            run_test_suite
            print_summary
            ;;
        "clean")
            clean_build
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        "all"|*)
            echo "Building Meadows Compiler..."
            build_project
            echo "✓ Build completed"
            echo
            run_test_suite
            print_summary
            ;;
    esac
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_color $RED "Error: This script must be run from the project root directory"
    exit 1
fi

# Run main function
main "$@"

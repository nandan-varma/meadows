#!/bin/bash

# Meadows Compiler Test Runner Script

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to print header
print_header() {
    echo
    print_color $BLUE "=================================="
    print_color $BLUE "$1"
    print_color $BLUE "=================================="
    echo
}

# Build the project
build_project() {
    print_header "Building Meadows Compiler"
    
    if [ ! -d "build" ]; then
        mkdir build
    fi
    
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    cd ..
    
    print_color $GREEN "✓ Build completed successfully"
}

# Run individual test files
run_individual_tests() {
    print_header "Running Individual Test Files"
    
    for test_file in tests/*.py; do
        if [ -f "$test_file" ]; then
            echo "Testing file: $test_file"
            if ./build/meadows -p "$test_file"; then
                print_color $GREEN "✓ $test_file passed"
            else
                print_color $RED "✗ $test_file failed"
            fi
            echo
        fi
    done
}

# Run the comprehensive test suite
run_test_suite() {
    print_header "Running Comprehensive Test Suite"
    
    if [ -f "./build/meadows_test" ]; then
        ./build/meadows_test
    else
        print_color $RED "Test runner not found. Make sure build completed successfully."
        exit 1
    fi
}

# Run original compiler tests
run_compiler_tests() {
    print_header "Running Built-in Compiler Tests"
    
    ./build/meadows  # This runs the demo and built-in tests
}

# Test specific examples
test_examples() {
    print_header "Testing Example Programs"
    
    local examples=("simple_example.py" "example.py" "test_program.py")
    
    for example in "${examples[@]}"; do
        if [ -f "$example" ]; then
            echo "Testing example: $example"
            if ./build/meadows -p "$example"; then
                print_color $GREEN "✓ $example parsed successfully"
            else
                print_color $RED "✗ $example failed to parse"
            fi
            echo
        fi
    done
}

# Test compilation to executable
test_compilation() {
    print_header "Testing Compilation to Executable"
    
    local test_files=("simple_example.py" "test_program.py")
    
    for test_file in "${test_files[@]}"; do
        if [ -f "$test_file" ]; then
            echo "Compiling: $test_file"
            local output_name=$(basename "$test_file" .py)
            
            if ./build/meadows -i "$test_file" -o "build/$output_name"; then
                print_color $GREEN "✓ $test_file compiled successfully"
                
                # Try to run the executable if it exists
                if [ -f "build/$output_name" ]; then
                    print_color $YELLOW "Running compiled executable..."
                    if ./build/$output_name; then
                        print_color $GREEN "✓ Executable ran successfully"
                    else
                        print_color $YELLOW "⚠ Executable completed with non-zero exit code"
                    fi
                fi
            else
                print_color $RED "✗ Failed to compile $test_file"
            fi
            echo
        fi
    done
}

# Show usage
show_usage() {
    echo "Usage: $0 [option]"
    echo
    echo "Options:"
    echo "  build       - Build the project"
    echo "  test        - Run comprehensive test suite"
    echo "  individual  - Run tests on individual test files"
    echo "  examples    - Test example programs"
    echo "  compile     - Test compilation to executable"
    echo "  compiler    - Run built-in compiler tests"
    echo "  all         - Run all tests (default)"
    echo "  clean       - Clean build directory"
    echo "  help        - Show this help message"
    echo
}

# Clean build directory
clean_build() {
    print_header "Cleaning Build Directory"
    
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
            build_project
            ;;
        "test")
            build_project
            run_test_suite
            ;;
        "individual")
            build_project
            run_individual_tests
            ;;
        "examples")
            build_project
            test_examples
            ;;
        "compile")
            build_project
            test_compilation
            ;;
        "compiler")
            build_project
            run_compiler_tests
            ;;
        "all")
            build_project
            run_compiler_tests
            echo
            run_individual_tests
            test_examples
            test_compilation
            run_test_suite
            ;;
        "clean")
            clean_build
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        *)
            print_color $RED "Unknown option: $command"
            show_usage
            exit 1
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

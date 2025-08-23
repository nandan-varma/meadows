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
TOTAL_ERRORS=0

# Category tracking using simple variables
CATEGORY_LIST=""
CATEGORY_STATS=""

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

# Helper functions for test statistics
update_test_stats() {
    local category=$1
    local passed=$2
    local failed=$3
    local errors=$4
    local total=$((passed + failed + errors))
    
    # Add to totals
    TOTAL_TESTS=$((TOTAL_TESTS + total))
    TOTAL_PASSED=$((TOTAL_PASSED + passed))
    TOTAL_FAILED=$((TOTAL_FAILED + failed + errors))
    
    # Add category to list if not already there
    if [[ ! "$CATEGORY_LIST" =~ "$category" ]]; then
        CATEGORY_LIST="$CATEGORY_LIST|$category"
    fi
    
    # Store category stats in a simple format: category:total:passed:failed
    CATEGORY_STATS="$CATEGORY_STATS|$category:$total:$passed:$((failed + errors))"
}

print_comprehensive_stats() {
    print_header "🌾 Meadows Compiler - Comprehensive Test Results"
    
    if [ $TOTAL_TESTS -eq 0 ]; then
        echo "📊 No tests were executed."
        echo "🏆 Quality Assessment: No data available"
        return
    fi
    
    # Overall statistics
    local success_rate=$(printf "%.1f" $(echo "scale=1; $TOTAL_PASSED * 100 / $TOTAL_TESTS" | bc -l))
    
    echo "📊 Overall Performance:"
    echo "  Total Tests: $TOTAL_TESTS"
    echo "  Passed: $TOTAL_PASSED ($(printf "%.1f" $(echo "scale=1; $TOTAL_PASSED * 100 / $TOTAL_TESTS" | bc -l))%)"
    echo "  Failed: $TOTAL_FAILED ($(printf "%.1f" $(echo "scale=1; $TOTAL_FAILED * 100 / $TOTAL_TESTS" | bc -l))%)"
    echo "  Success Rate: ${success_rate}%"
    echo
    
    # Category breakdown
    if [ -n "$CATEGORY_STATS" ]; then
        echo "📂 Test Categories Breakdown:"
        echo "------------------------------------------------------------"
        printf "%-25s %-8s %-12s %-18s\n" "Category" "Count" "Percentage" "Visual"
        echo "------------------------------------------------------------"
        
        # Process category stats
        IFS='|' read -ra CATEGORIES <<< "$CATEGORY_STATS"
        for cat_stat in "${CATEGORIES[@]}"; do
            if [ -n "$cat_stat" ]; then
                IFS=':' read -ra STAT_PARTS <<< "$cat_stat"
                local category="${STAT_PARTS[0]}"
                local count="${STAT_PARTS[1]}"
                local passed="${STAT_PARTS[2]}"
                local failed="${STAT_PARTS[3]}"
                
                if [ -n "$category" ] && [ -n "$count" ] && [ "$count" -gt 0 ]; then
                    local percentage=$(printf "%.1f" $(echo "scale=1; $count * 100 / $TOTAL_TESTS" | bc -l))
                    local pass_rate=$(printf "%.0f" $(echo "scale=0; $passed * 100 / $count" | bc -l))
                    
                    # Create visual bar
                    local bar_width=15
                    local filled_bars=$(printf "%.0f" $(echo "scale=0; $pass_rate * $bar_width / 100" | bc -l))
                    local bar="["
                    for ((i=1; i<=bar_width; i++)); do
                        if [ $i -le $filled_bars ]; then
                            bar+="█"
                        else
                            bar+="░"
                        fi
                    done
                    bar+="]"
                    
                    printf "%-25s %-8s %-12s %-18s\n" "$category" "$count" "${percentage}%" "$bar"
                fi
            fi
        done
        echo "------------------------------------------------------------"
        echo
    fi
    
    # Results breakdown
    echo "🎯 Results Breakdown:"
    echo "  ✅ Passed: $TOTAL_PASSED ($(printf "%.1f" $(echo "scale=1; $TOTAL_PASSED * 100 / $TOTAL_TESTS" | bc -l))%)"
    echo "  ❌ Failed: $TOTAL_FAILED ($(printf "%.1f" $(echo "scale=1; $TOTAL_FAILED * 100 / $TOTAL_TESTS" | bc -l))%)"
    echo
    
    # Progress bar
    echo -n "Progress: ["
    local bar_width=50
    local passed_bars=$(printf "%.0f" $(echo "scale=0; $TOTAL_PASSED * $bar_width / $TOTAL_TESTS" | bc -l))
    local failed_bars=$(printf "%.0f" $(echo "scale=0; $TOTAL_FAILED * $bar_width / $TOTAL_TESTS" | bc -l))
    
    for ((i=1; i<=bar_width; i++)); do
        if [ $i -le $passed_bars ]; then
            echo -n "✓"
        elif [ $i -le $((passed_bars + failed_bars)) ]; then
            echo -n "✗"
        else
            echo -n "·"
        fi
    done
    echo "] ${success_rate}%"
    echo
    
    # Quality assessment
    echo "🏆 Quality Assessment:"
    local success_int=$(printf "%.0f" $success_rate)
    if [ $success_int -ge 95 ]; then
        print_color $GREEN "  🌟 Excellent test coverage and quality!"
    elif [ $success_int -ge 85 ]; then
        print_color $YELLOW "  ✅ Good test quality, minor issues to address"
    elif [ $success_int -ge 70 ]; then
        print_color $YELLOW "  ⚠️  Moderate test quality, needs improvement"
    else
        print_color $RED "  ❌ Poor test quality, significant issues need attention"
    fi
    echo
    
    # Final result
    if [ $TOTAL_FAILED -eq 0 ]; then
        print_color $GREEN "🎉 All tests passed!"
    else
        print_color $RED "❌ Some tests failed."
    fi
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
        # Run test framework in quiet mode and capture exit code
        local framework_exit_code=0
        ./build/meadows_test "./tests/" quiet || framework_exit_code=$?
        
        # Get stats from test framework by parsing its output when run without quiet mode
        # We'll temporarily run it again just to get stats
        local temp_output=$(./build/meadows_test "./tests/" 2>/dev/null | grep -E "(Total tests:|Passed:|Failed:|Errors:)" || true)
        
        if [ -n "$temp_output" ]; then
            local framework_total=$(echo "$temp_output" | grep "Total tests:" | grep -o '[0-9]*' | head -1)
            local framework_passed=$(echo "$temp_output" | grep "Passed:" | grep -o '[0-9]*' | head -1)
            local framework_failed=$(echo "$temp_output" | grep "Failed:" | grep -o '[0-9]*' | head -1)
            local framework_errors=$(echo "$temp_output" | grep "Errors:" | grep -o '[0-9]*' | head -1)
            
            # Default to 0 if parsing failed
            framework_total=${framework_total:-0}
            framework_passed=${framework_passed:-0}
            framework_failed=${framework_failed:-0}
            framework_errors=${framework_errors:-0}
            
            update_test_stats "Test Framework" $framework_passed $framework_failed $framework_errors
        fi
        
        # Generate HTML report
        print_color $BLUE "Generating comprehensive HTML test report..."
        ./build/meadows_test "./tests/" 2>/dev/null | grep -q "HTML report generated" || true
        
    else
        print_color $RED "Test runner not found. Make sure build completed successfully."
        update_test_stats "Test Framework" 0 1 0
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
    local passed=0
    local failed=0
    
    for example in "${examples[@]}"; do
        if [ -f "$example" ]; then
            echo "Testing example: $example"
            if ./build/meadows -p "$example" >/dev/null 2>&1; then
                print_color $GREEN "✓ $example parsed successfully"
                ((passed++))
            else
                print_color $RED "✗ $example failed to parse"
                ((failed++))
            fi
            echo
        fi
    done
    
    update_test_stats "Example Programs" $passed $failed 0
}

# Test compilation to executable
test_compilation() {
    print_header "Testing Compilation to Executable"
    
    local test_files=("simple_example.py" "test_program.py" "tests/test_compilation.py" "tests/test_integration.py")
    
    for test_file in "${test_files[@]}"; do
        if [ -f "$test_file" ]; then
            echo "Compiling: $test_file"
            local output_name=$(basename "$test_file" .py)
            
            if ./build/meadows -i "$test_file" -o "build/$output_name"; then
                print_color $GREEN "✓ $test_file compiled successfully"
                
                # Try to run the executable if it exists
                if [ -f "build/$output_name" ]; then
                    print_color $YELLOW "Running compiled executable..."
                    if timeout 30 ./build/$output_name; then
                        print_color $GREEN "✓ Executable ran successfully"
                    else
                        print_color $YELLOW "⚠ Executable completed with non-zero exit code or timeout"
                    fi
                fi
            else
                print_color $RED "✗ Failed to compile $test_file"
            fi
            echo
        fi
    done
}

# Test new comprehensive test categories
test_comprehensive_categories() {
    print_header "Running Comprehensive Test Categories"
    
    local categories=("compilation" "error_handling" "performance" "ir_generation" "integration" "applications")
    
    for category in "${categories[@]}"; do
        if [ -f "tests/test_${category}.py" ]; then
            echo "Testing category: $category"
            if ./build/meadows -p "tests/test_${category}.py"; then
                print_color $GREEN "✓ $category tests passed"
            else
                print_color $RED "✗ $category tests failed"
            fi
            echo
        fi
    done
}

# Test error handling and edge cases
test_error_handling() {
    print_header "Testing Error Handling and Edge Cases"
    
    echo "Testing syntax error handling..."
    if [ -f "tests/test_error_handling.py" ]; then
        # This should fail to parse due to syntax errors
        if ./build/meadows -p "tests/test_error_handling.py" 2>/dev/null; then
            print_color $YELLOW "⚠ Error handling test compiled unexpectedly"
        else
            print_color $GREEN "✓ Error handling working correctly"
        fi
    fi
    
    echo "Testing graceful error recovery..."
    # Test that compiler doesn't crash on invalid input
    echo "def invalid_syntax(" | ./build/meadows -p - 2>/dev/null
    if [ $? -ne 0 ]; then
        print_color $GREEN "✓ Compiler handles invalid input gracefully"
    else
        print_color $YELLOW "⚠ Unexpected success on invalid input"
    fi
}

# Performance and stress testing
test_performance() {
    print_header "Running Performance and Stress Tests"
    
    if [ -f "tests/test_performance.py" ]; then
        echo "Testing compilation performance..."
        local start_time=$(date +%s.%N)
        
        if ./build/meadows -p "tests/test_performance.py"; then
            local end_time=$(date +%s.%N)
            local duration=$(echo "$end_time - $start_time" | bc -l)
            
            print_color $GREEN "✓ Performance test completed in ${duration}s"
            
            # Check if compilation time is reasonable (under 10 seconds)
            if (( $(echo "$duration < 10" | bc -l) )); then
                print_color $GREEN "✓ Compilation time within acceptable limits"
            else
                print_color $YELLOW "⚠ Compilation took longer than expected"
            fi
        else
            print_color $RED "✗ Performance test failed"
        fi
    fi
}

# Test LLVM IR generation
test_ir_generation() {
    print_header "Testing LLVM IR Generation"
    
    if [ -f "tests/test_ir_generation.py" ]; then
        echo "Testing IR generation..."
        if ./build/meadows -p "tests/test_ir_generation.py"; then
            print_color $GREEN "✓ IR generation test passed"
            
            # Test compilation with different optimization levels if supported
            echo "Testing with optimization..."
            if ./build/meadows -p "tests/test_ir_generation.py" -O2 2>/dev/null; then
                print_color $GREEN "✓ Optimized compilation succeeded"
            else
                print_color $YELLOW "⚠ Optimization not supported or failed"
            fi
        else
            print_color $RED "✗ IR generation test failed"
        fi
    fi
}

# Real-world application testing
test_applications() {
    print_header "Testing Real-World Applications"
    
    if [ -f "tests/test_applications.py" ]; then
        echo "Testing application examples..."
        if ./build/meadows -p "tests/test_applications.py"; then
            print_color $GREEN "✓ Application tests passed"
            
            # Try to compile to executable
            echo "Compiling application to executable..."
            if ./build/meadows -i "tests/test_applications.py" -o "build/test_app"; then
                print_color $GREEN "✓ Application compiled to executable"
                
                # Try to run the application
                if [ -f "build/test_app" ]; then
                    echo "Running compiled application..."
                    if timeout 30 ./build/test_app; then
                        print_color $GREEN "✓ Application executed successfully"
                    else
                        print_color $YELLOW "⚠ Application execution failed or timed out"
                    fi
                fi
            else
                print_color $YELLOW "⚠ Application compilation to executable failed"
            fi
        else
            print_color $RED "✗ Application tests failed"
        fi
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 [option]"
    echo
    echo "Options:"
    echo "  build         - Build the project"
    echo "  test          - Run comprehensive test suite"
    echo "  individual    - Run tests on individual test files"
    echo "  examples      - Test example programs"
    echo "  compile       - Test compilation to executable"
    echo "  compiler      - Run built-in compiler tests"
    echo "  comprehensive - Run comprehensive test categories"
    echo "  errors        - Test error handling and edge cases"
    echo "  performance   - Run performance and stress tests"
    echo "  ir            - Test LLVM IR generation"
    echo "  applications  - Test real-world applications"
    echo "  all           - Run all tests (default)"
    echo "  clean         - Clean build directory"
    echo "  help          - Show this help message"
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
            print_comprehensive_stats
            ;;
        "individual")
            build_project
            run_individual_tests
            ;;
        "examples")
            build_project
            test_examples
            print_comprehensive_stats
            ;;
        "compile")
            build_project
            test_compilation
            ;;
        "compiler")
            build_project
            run_compiler_tests
            ;;
        "comprehensive")
            build_project
            test_comprehensive_categories
            ;;
        "errors")
            build_project
            test_error_handling
            ;;
        "performance")
            build_project
            test_performance
            ;;
        "ir")
            build_project
            test_ir_generation
            ;;
        "applications")
            build_project
            test_applications
            ;;
        "all")
            build_project
            run_compiler_tests
            echo
            run_individual_tests
            test_examples
            test_compilation
            test_comprehensive_categories
            test_error_handling
            test_performance
            test_ir_generation
            test_applications
            run_test_suite
            
            print_comprehensive_stats
            print_color $CYAN "HTML report available at: build/test_report.html"
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

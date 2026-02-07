#!/bin/bash

echo "========================================"
echo "   Meadows Compiler Integration Tests"
echo "========================================"
echo ""

PASSED=0
FAILED=0

for test_file in tests/integration/*.ms; do
    if [ -f "$test_file" ]; then
        basename=$(basename "$test_file" .ms)
        expected_file="tests/integration/expected/${basename}.expected"
        output_file="/tmp/${basename}.output"
        
        if [ ! -f "$expected_file" ]; then
            echo "⚠ $basename - no expected file"
            continue
        fi
        
        # Compile
        ./build/bin/meadows "$test_file" > /dev/null 2>&1
        compile_result=$?
        
        if [ $compile_result -ne 0 ]; then
            echo "✗ $basename - compilation failed"
            FAILED=$((FAILED + 1))
            continue
        fi
        
        # Run
        if [ -f "${test_file}.out" ]; then
            ./"${test_file}.out" > "$output_file" 2>&1
            
            # Compare
            if diff -q "$output_file" "$expected_file" > /dev/null 2>&1; then
                echo "✓ $basename"
                PASSED=$((PASSED + 1))
            else
                echo "✗ $basename - output mismatch"
                echo "Expected:"
                cat "$expected_file"
                echo "Got:"
                cat "$output_file"
                FAILED=$((FAILED + 1))
            fi
        else
            echo "✗ $basename - executable not generated"
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo ""
echo "========================================"
echo "Results: $PASSED passed, $FAILED failed"
echo "========================================"

if [ $FAILED -gt 0 ]; then
    exit 1
fi

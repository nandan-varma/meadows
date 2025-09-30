#!/bin/bash

# Build the project
./build.sh

if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

# Clean up any existing generated files in tests/
rm -f tests/*.ll tests/*.out

total_tests=0
passed=0

for file in tests/*.ms; do
    expected_file="${file}.expected"
    if [ ! -f "$expected_file" ]; then
        continue
    fi
    ((total_tests++))
    echo -n "Testing $(basename "$file"): "
    ./build/meadows "$file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "COMPILATION FAILED"
        continue
    fi
    output=$(./"${file}.out")
    expected=$(cat "$expected_file")
    if [ "$output" == "$expected" ]; then
        echo "PASS"
        ((passed++))
    else
        echo "FAIL"
        echo "  Expected: '$expected'"
        echo "  Got:      '$output'"
    fi
    # Clean up generated files
    rm -f "${file}.ll" "${file}.out"
done

if [ $total_tests -eq 0 ]; then
    echo "No test cases found (missing .expected files)"
    exit 1
fi

percentage=$((passed * 100 / total_tests))
echo "Total: $passed/$total_tests tests passed ($percentage%)"
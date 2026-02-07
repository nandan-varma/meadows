#!/bin/bash

# Performance Benchmark Script for Meadows Compiler
# Measures compilation speed for various file sizes

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_ROOT/build/bin/meadows"
RESULTS_FILE="$PROJECT_ROOT/build/benchmark_results.txt"

echo "=== Meadows Compiler Performance Benchmarks ===" > "$RESULTS_FILE"
echo "Date: $(date)" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

generate_source() {
    local lines=$1
    local chars_per_line=$2
    local output_file=$3
    
    {
        for ((i=0; i<lines; i++)); do
            for ((j=0; j<chars_per_line; j++)); do
                echo -n "let x${j}_${i} = ${j} + ${i}; "
            done
            echo
        done
    } > "$output_file"
}

run_benchmark() {
    local name=$1
    local file=$2
    local iterations=${3:-3}
    
    echo "Running: $name"
    
    local total_time=0
    local min_time=999999
    local max_time=0
    
    for ((i=0; i<iterations; i++)); do
        local start=$(date +%s.%N)
        $COMPILER "$file" > /dev/null 2>&1
        local end=$(date +%s.%N)
        
        local time=$(echo "$end - $start" | bc)
        total_time=$(echo "$total_time + $time" | bc)
        
        if (( $(echo "$time < $min_time" | bc -l) )); then
            min_time=$time
        fi
        if (( $(echo "$time > $max_time" | bc -l) )); then
            max_time=$time
        fi
    done
    
    local avg_time=$(echo "scale=6; $total_time / $iterations" | bc)
    
    echo "  Iterations: $iterations"
    echo "  Average: ${avg_time}s"
    echo "  Min: ${min_time}s"
    echo "  Max: ${max_time}s"
    echo "  Throughput: $(echo "scale=2; $(wc -c < "$file") / $avg_time / 1024" | bc) KB/s"
    echo "" >> "$RESULTS_FILE"
    echo "Benchmark: $name" >> "$RESULTS_FILE"
    echo "  File size: $(wc -c < "$file") bytes" >> "$RESULTS_FILE"
    echo "  Iterations: $iterations" >> "$RESULTS_FILE"
    echo "  Average: ${avg_time}s" >> "$RESULTS_FILE"
    echo "  Throughput: $(echo "scale=2; $(wc -c < "$file") / $avg_time / 1024" | bc) KB/s" >> "$RESULTS_FILE"
}

echo ""
echo "Generating test files..."

# Create temporary files
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

generate_source 10 50 "$TEMP_DIR/small.ms"      # ~5KB
generate_source 100 50 "$TEMP_DIR/medium.ms"    # ~50KB  
generate_source 500 50 "$TEMP_DIR/large.ms"     # ~250KB
generate_source 1000 100 "$TEMP_DIR/xlarge.ms" # ~1MB

echo "  Small (5KB): $TEMP_DIR/small.ms"
echo "  Medium (50KB): $TEMP_DIR/medium.ms"
echo "  Large (250KB): $TEMP_DIR/large.ms"
echo "  XLarge (1MB): $TEMP_DIR/xlarge.ms"

echo ""
echo "Running benchmarks..."
echo ""

if [ ! -f "$COMPILER" ]; then
    echo "Error: Compiler not found at $COMPILER"
    echo "Please build first: ./build.sh"
    exit 1
fi

run_benchmark "Small file (5KB)" "$TEMP_DIR/small.ms" 5
run_benchmark "Medium file (50KB)" "$TEMP_DIR/medium.ms" 3
run_benchmark "Large file (250KB)" "$TEMP_DIR/large.ms" 3
run_benchmark "XLarge file (1MB)" "$TEMP_DIR/xlarge.ms" 2

echo ""
echo "Benchmark complete! Results saved to: $RESULTS_FILE"
echo ""
cat "$RESULTS_FILE"

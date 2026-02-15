# Performance Test Suite for Meadows Compiler

## Overview
This directory contains performance benchmarks and regression tests to ensure
the compiler meets performance requirements and does not degrade over time.

## Benchmark Categories

### Compilation Benchmarks
- **fibonacci_bench.ms**: Recursive computation benchmark
- **matrix_mult_bench.ms**: Numeric computation benchmark
- **string_ops_bench.ms**: String manipulation benchmark
- **sort_bench.ms**: Algorithm complexity benchmark

### Scalability Tests
- **large_file_*.ms**: Large source file handling (1MB, 10MB)
- **many_functions.ms**: Projects with 1000+ functions
- **deep_ast.ms**: Deeply nested expressions (100+ levels)

### Memory Benchmarks
- **memory_usage.ms**: Memory consumption tracking
- **allocation_patterns.ms**: Allocation efficiency
- **gc_pressure.ms**: Garbage collection performance

## Running Benchmarks

```bash
# Run all benchmarks
./test.sh bench

# Run specific benchmark
./build/tests/meadows_tests "[performance]"

# Run with timing
./build/tests/meadows_tests "[performance]" -d yes
```

## Performance Requirements

| Metric | Target | Maximum |
|--------|--------|---------|
| Compile 1000 LOC | < 1s | < 5s |
| Fibonacci(35) | < 1s | < 5s |
| Memory per 1000 LOC | < 10MB | < 50MB |
| Binary size | < 100KB | < 500KB |

## Regression Testing

Performance tests compare against baseline metrics:
- Any regression > 10% fails the test
- Results stored in `.benchmark_results/`
- CI/CD tracks performance over time

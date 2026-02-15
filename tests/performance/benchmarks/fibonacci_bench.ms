# Benchmark: fibonacci_bench.ms
# Purpose: Measure recursive function call performance
# Target: fib(35) < 1 second

func fib(n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

func main() {
    # Benchmark: Calculate fib(35)
    # Expected: ~9 million function calls
    let result = fib(35);
    print result;  # Should be 9227465
    return 0;
}

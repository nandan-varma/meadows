// Test: integer_overflow.ms
// Purpose: Verify integer overflow detection
// Expected: Compiler/runtime should handle or report overflow

# Test positive overflow
let max_val = 2147483647;
let overflow = max_val + 1;  // Should detect/report overflow

# Test negative overflow  
let min_val = -2147483648;
let underflow = min_val - 1;  // Should detect/report underflow

# Test multiplication overflow
let big_mult = 100000 * 100000;  // 10^10 exceeds 32-bit

func main() {
    print max_val;
    return 0;
}

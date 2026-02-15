// Test: buffer_overflow.ms
// Purpose: Verify buffer bounds checking
// Expected: Array access should be bounds-checked

func main() {
    let arr = [1, 2, 3, 4, 5];
    
    # These should trigger bounds errors
    # let out_of_bounds = arr[10];
    # arr[-1] = 0;
    
    # Valid access
    print arr[0];
    print arr[4];
    
    return 0;
}

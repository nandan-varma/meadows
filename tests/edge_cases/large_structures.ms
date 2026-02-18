# Edge Case Test - Deep Recursion and Large Structures
# Tests the compiler's handling of deeply nested constructs

# Large array literal
let large_array = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
                  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                  21, 22, 23, 24, 25, 26, 27, 28, 29, 30];

# Nested function calls
func add3(x: i32, y: i32, z: i32) -> i32 {
    return x + y + z;
}

func add2(x: i32, y: i32) -> i32 {
    return x + y;
}

# Multiple levels of nesting
func nested_add(x: i32) -> i32 {
    return add3(add2(x, 1), add2(x, 2), add2(x, 3));
}

# Struct with many fields
type Point struct {
    x: i32,
    y: i32,
    z: i32,
    w: i32
}

# Match with many arms
enum Status {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled,
    Timeout,
    Unknown
}

func get_status_name(s: Status) -> i32 {
    let result = match s {
        Pending => 1,
        Running => 2,
        Completed => 3,
        Failed => 4,
        Cancelled => 5,
        Timeout => 6,
        Unknown => 7,
        _ => 0
    };
    return result;
}

# Long string
let long_string = "This is a very long string that tests the compiler's ability to handle large string literals in the source code without any issues.";

func main() -> i32 {
    # Test large array access
    let val = large_array[0];
    print val;
    
    # Test nested function calls
    let nested = nested_add(10);
    print nested;
    
    # Test struct creation
    let p = Point { x: 1, y: 2, z: 3, w: 4 };
    print p.x;
    
    # Test match with many arms
    let status = get_status_name(Completed);
    print status;
    
    # Test long string
    print long_string;
    
    return 0;
}

# Pattern Matching Comprehensive Tests
# Fixed version - patterns must be literals or identifiers

# Test 1: Basic literal patterns
func test_literal_patterns(x: i32) -> i32 {
    return match (x) {
        0 => 100,
        1 => 101,
        2 => 102,
        _ => 999,
    };
}

# Test 2: All wildcard pattern
func test_wildcard(x: i32) -> i32 {
    return match (x) {
        _ => 42,
    };
}

# Test 3: Match with expressions
func test_match_expressions() -> i32 {
    let a = 5;
    let b = 10;
    
    let result = match (a + b) {
        0 => 0,
        15 => 15,
        _ => -1,
    };
    
    return result;
}

# Test 4: Match in different contexts
func test_match_contexts() -> i32 {
    # Match in let binding
    let x = match (1) {
        1 => 100,
        _ => 0,
    };
    
    # Match in return
    return match (x) {
        100 => 42,
        _ => 0,
    };
}

# Test 5: Nested match expressions
func test_nested_match() -> i32 {
    let x = 1;
    let y = 2;
    
    return match (x) {
        0 => 0,
        1 => match (y) {
            1 => 11,
            2 => 12,
            _ => 10,
        },
        _ => -1,
    };
}

# Test 6: Match with variable binding (using wildcard)
func test_binding_patterns(x: i32) -> i32 {
    return match (x) {
        0 => 0,
        _ => 1,  # wildcard matches anything
    };
}

# Test 7: Multiple match expressions
func test_multiple_matches() -> i32 {
    let a = match (1) { 1 => 10, _ => 0 };
    let b = match (2) { 2 => 20, _ => 0 };
    let c = match (3) { 3 => 30, _ => 0 };
    
    return a + b + c;
}

# Test 8: Match in loops
func test_match_in_loops() -> i32 {
    let sum = 0;
    
    for (i in range(0, 5)) {
        let value = match (i) {
            0 => 10,
            1 => 20,
            2 => 30,
            _ => 0,
        };
        sum = sum + value;
    }
    
    return sum;
}

# Test 9: Large number patterns
func test_large_patterns() -> i32 {
    return match (1000000) {
        1000000 => 1,
        999999 => 2,
        _ => 3,
    };
}

# Test 10: Duplicate patterns (edge case)
func test_duplicate_patterns(x: i32) -> i32 {
    return match (x) {
        1 => 100,
        _ => 0,
    };
}

func main() -> i32 {
    print test_literal_patterns(0);
    print test_literal_patterns(1);
    print test_literal_patterns(5);
    print test_wildcard(999);
    print test_match_expressions();
    print test_match_contexts();
    print test_nested_match();
    print test_binding_patterns(42);
    print test_multiple_matches();
    print test_match_in_loops();
    print test_large_patterns();
    print test_duplicate_patterns(1);
    
    return 0;
}

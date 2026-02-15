# Parser Edge Case Tests
# Comprehensive tests for parser edge cases

# Test 1: Empty block
func empty_block() -> i32 {
    return 0;
}

# Test 2: Nested blocks
func nested_blocks() -> i32 {
    {
        {
            {
                return 42;
            }
        }
    }
}

# Test 3: Multiple semicolons
func multiple_semicolons() -> i32 {
    let x = 1;;
    let y = 2;;;
    return x + y;
}

# Test 4: Empty statements (just semicolons)
func empty_statements() -> i32 {
    ;
    ;
    let x = 1;
    ;
    return x;
}

# Test 5: Complex nested expressions
func complex_expressions() -> i32 {
    let a = 1 + 2 * 3 - 4 / 5;
    let b = (1 + 2) * (3 - 4) / 5;
    let c = 1 + (2 + (3 + (4 + 5)));
    let d = ((1));
    let e = 1 * 2 + 3 * 4 + 5 * 6;
    return a + b + c + d + e;
}

# Test 6: Operator precedence edge cases
func precedence_tests() -> i32 {
    let a = 1 + 2 * 3;      # Should be 7, not 9
    let b = 1 * 2 + 3;      # Should be 5
    let c = 10 - 5 - 2;     # Should be 3, not 7
    let d = 20 / 4 / 2;     # Should be 2.5 or 2
    return a + b + c;
}

# Test 7: Complex if-else nesting
func complex_if_else(x: i32) -> i32 {
    if (x > 0) {
        if (x > 10) {
            return 1;
        } else {
            return 2;
        }
    } else {
        if (x < -10) {
            return -1;
        } else {
            return -2;
        }
    }
}

# Test 8: Chained if-else-if
func chained_if(x: i32) -> i32 {
    if (x == 1) {
        return 1;
    } else if (x == 2) {
        return 2;
    } else if (x == 3) {
        return 3;
    } else {
        return 0;
    }
}

# Test 9: For loop edge cases
func for_loop_tests() -> i32 {
    # Normal for loop
    let sum1 = 0;
    for (i in range(0, 10)) {
        sum1 = sum1 + i;
    }
    
    # Single iteration
    let sum2 = 0;
    for (i in range(0, 1)) {
        sum2 = sum2 + i;
    }
    
    # Zero iterations
    let sum3 = 0;
    for (i in range(10, 0)) {
        sum3 = sum3 + i;
    }
    
    return sum1 + sum2 + sum3;
}

# Test 10: While loop edge cases
func while_tests() -> i32 {
    # Normal while
    let i = 0;
    let sum = 0;
    while (i < 10) {
        sum = sum + i;
        i = i + 1;
    }
    
    return sum;
}

# Test 11: Array edge cases
func array_tests() -> i32 {
    let empty_arr = [];
    let single = [1];
    let nested = [[1, 2], [3, 4]];
    let large = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    
    return single[0];
}

# Test 12: Object/struct edge cases
func object_tests() -> i32 {
    let empty = {};
    let single = {a: 1};
    let nested = {a: {b: {c: 1}}};
    let many = {a: 1, b: 2, c: 3, d: 4, e: 5};
    
    return single.a;
}

# Test 13: Function with no return value
func no_return() {
    let x = 1;
    print x;
}

# Test 14: Deep recursion (if supported)
func recursive_sum(n: i32) -> i32 {
    if (n <= 0) {
        return 0;
    }
    return n + recursive_sum(n - 1);
}

# Test 15: Variable shadowing
func shadowing_test() -> i32 {
    let x = 1;
    {
        let x = 2;
        {
            let x = 3;
            print x;
        }
        print x;
    }
    print x;
    return x;
}

# Test 16: Multiple assignments
func multi_assign() -> i32 {
    let a = 1;
    let b = 2;
    let c = 3;
    a = b = c = 10;
    return a;
}

# Test 17: Expression statements
func expr_statements() -> i32 {
    1 + 2;
    3 * 4;
    "hello";
    return 42;
}

# Test 18: Empty function
func empty_func() -> i32 {
    return 0;
}

# Test 19: Single line function
func single_line() -> i32 { return 42; }

# Test 20: Very long parameter list
func many_params(a: i32, b: i32, c: i32, d: i32, e: i32, f: i32, g: i32, h: i32) -> i32 {
    return a + b + c + d + e + f + g + h;
}

func main() -> i32 {
    print 42;
    return 0;
}

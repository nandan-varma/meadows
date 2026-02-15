# Lexer Edge Case Tests
# Comprehensive tests for lexer edge cases and boundary conditions

# Test 1: Empty string literal
let empty = "";
print empty;

# Test 2: String with only whitespace
let whitespace = "   ";
print whitespace;

# Test 3: All escape sequences
let escapes = "\n\t\r\0\\\"\'\b\f";
print escapes;

# Test 4: Very long string
let long_string = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
print long_string;

# Test 5: Numbers with various formats
let num1 = 0;
let num2 = 00;
let num3 = 000123;
let num4 = 1.0;
let num5 = 0.0;
let num6 = .5;
let num7 = 1.;
let num8 = 1e10;
let num9 = 1.5e-5;

print num1;
print num2;
print num3;

# Test 6: Keywords as identifiers (edge case)
let let1 = 1;
let func1 = 2;
let if1 = 3;
let else1 = 4;
let while1 = 5;
let for1 = 6;
let return1 = 7;
let print1 = 8;

print let1;
print func1;
print if1;

# Test 7: Mixed tabs and spaces (indentation edge case)
func test_indent() -> i32 {
    let x = 1;
    let y = 2;
    return x + y;
}

# Test 8: Comments in various positions
let a = 1; # trailing comment
# comment before statement
let b = 2;
let c = 3; # another trailing

# Test 9: Adjacent operators
let d = 1+++2;  # valid in some languages
let e = 1---2;  # same here

# Test 10: Maximum integer value
let max_int = 2147483647;
print max_int;

# Test 11: Negative numbers
let neg1 = -1;
let neg2 = -2147483648;

print neg1;

# Test 12: Zero padding edge cases
let zero1 = 007;
let zero2 = 00.5;

print zero1;
print zero2;

# Test 13: Identifier edge cases
let _ = 1;
let _a = 2;
let a_ = 3;
let _a_ = 4;
let a1b2c3 = 5;
let camelCase = 6;
let snake_case = 7;

print _;
print _a;
print a_;
print a1b2c3;

# Test 14: Unicode in strings (if supported)
# let unicode = "Hello 世界 🌍";

func main() -> i32 {
    print 42;
    return 0;
}

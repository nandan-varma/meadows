# Error Handling and Recovery Tests

# Test 1: Syntax errors that should be caught
# These should all fail to parse

# Missing colon in function definition
def invalid_function()
    return 42

# Mismatched parentheses
def unmatched_parens():
    result = (10 + 20 * 30
    return result

# Invalid indentation
def bad_indentation():
return 42

# Missing comma in parameter list
def bad_params(a b c):
    return a + b + c

# Invalid assignment
x + y = 10

# Unclosed string literal
message = "This string is not closed

# Invalid operator usage
result = 10 +* 20

# Missing function body
def empty_function():

# Invalid class definition
class BadClass
    def method(self):
        return 42

# Nested function with syntax error
def outer():
    def inner(
        return 10
    return inner()

# Test 2: Semantic errors (if implemented)

# Undefined variable usage
def undefined_var():
    return unknown_variable

# Function with wrong number of arguments
def wrong_args():
    def helper(a, b):
        return a + b
    
    return helper(10)  # Missing second argument

# Return outside function
return 42

# Test 3: Type-related errors (if type checking implemented)

# Type mismatch operations
def type_errors():
    number = 42
    text = "hello"
    
    # These should cause type errors if type checking is implemented
    result1 = number + text
    result2 = text * "world"
    result3 = 10 / "zero"
    
    return result1

# Test 4: Runtime error scenarios

# Division by zero
def division_by_zero():
    x = 10
    y = 0
    return x / y

# Recursive function without base case (stack overflow)
def infinite_recursion(n):
    return infinite_recursion(n + 1)

# Test 5: Edge cases that might cause issues

# Very long identifier names
very_long_identifier_name_that_might_cause_issues_with_parsing_or_symbol_table_management = 42

# Deep nesting
def deeply_nested():
    if True:
        if True:
            if True:
                if True:
                    if True:
                        if True:
                            if True:
                                if True:
                                    if True:
                                        if True:
                                            return 42

# Complex expression with many operators
def complex_expression():
    return 1 + 2 * 3 - 4 / 5 + 6 ** 7 % 8 and True or False and not True

# Test 6: Memory and resource stress tests

# Large number of variables
def many_variables():
    var1 = 1
    var2 = 2
    var3 = 3
    var4 = 4
    var5 = 5
    var6 = 6
    var7 = 7
    var8 = 8
    var9 = 9
    var10 = 10
    var11 = 11
    var12 = 12
    var13 = 13
    var14 = 14
    var15 = 15
    var16 = 16
    var17 = 17
    var18 = 18
    var19 = 19
    var20 = 20
    
    return var1 + var2 + var3 + var4 + var5 + var6 + var7 + var8 + var9 + var10 + var11 + var12 + var13 + var14 + var15 + var16 + var17 + var18 + var19 + var20

# Very long function with many statements
def long_function():
    x = 1
    x = x + 1
    x = x * 2
    x = x - 1
    x = x / 2
    x = x + 1
    x = x * 2
    x = x - 1
    x = x / 2
    x = x + 1
    x = x * 2
    x = x - 1
    x = x / 2
    x = x + 1
    x = x * 2
    x = x - 1
    x = x / 2
    x = x + 1
    x = x * 2
    x = x - 1
    x = x / 2
    return x

# Test 7: Unicode and special characters (if supported)

# Unicode in identifiers
def тест_unicode():
    привет = "hello"
    return привет

# Special characters in strings
def special_chars():
    newline = "hello\nworld"
    tab = "hello\tworld"
    quote = "hello\"world"
    backslash = "hello\\world"
    return newline + tab + quote + backslash

# Test 8: Boundary value testing

# Maximum integer values
def max_values():
    big_number = 999999999999999999999999999999
    small_number = -999999999999999999999999999999
    return big_number + small_number

# Floating point edge cases
def float_edge_cases():
    very_small = 0.000000000000000001
    very_large = 999999999999999999.999999999999999999
    return very_small + very_large

# Test 9: Comment edge cases

def comments_test():
    # This is a normal comment
    x = 10  # Inline comment
    # Multiple
    # Line
    # Comments
    return x

# Test 10: Empty and minimal cases

# Empty function
def empty():
    pass

# Minimal program
x = 1

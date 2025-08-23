# LLVM IR Generation and Code Quality Tests

# Test 1: Basic function that should generate clean LLVM IR
def simple_function(x):
    """Test basic function IR generation"""
    return x + 42

# Test 2: Function with multiple basic blocks (control flow)
def control_flow_ir(n):
    """Test IR generation for control flow structures"""
    if n > 0:
        result = n * 2
    else:
        result = n * -1
    
    return result

# Test 3: Loop IR generation
def loop_ir():
    """Test IR generation for loops"""
    total = 0
    i = 0
    
    while i < 10:
        total = total + i
        i = i + 1
    
    return total

# Test 4: Function calls and stack management
def function_calls_ir():
    """Test IR for function calls and parameter passing"""
    
    def helper(a, b):
        return a * b + 10
    
    result1 = helper(5, 6)
    result2 = helper(result1, 2)
    
    return result2

# Test 5: Class and method IR generation
class IRTestClass:
    """Test IR generation for classes and methods"""
    
    def __init__(self, value):
        self.value = value
    
    def get_value(self):
        return self.value
    
    def set_value(self, new_value):
        self.value = new_value
        return self.value
    
    def compute(self, multiplier):
        self.value = self.value * multiplier
        return self.value + 100

# Test 6: Complex arithmetic for optimization testing
def optimization_candidate():
    """Test function that should benefit from optimizations"""
    
    # Dead code that should be eliminated
    unused_var = 42 * 17 + 33
    
    # Constant folding opportunities
    constant_expr = 10 + 20 * 30 / 5 - 2
    
    # Common subexpression elimination
    x = 15
    y = 25
    result1 = x * y + 100
    result2 = x * y + 200
    result3 = x * y + 300
    
    # Loop that could be unrolled or vectorized
    sum_val = 0
    i = 0
    while i < 4:
        sum_val = sum_val + i * 2
        i = i + 1
    
    return result1 + result2 + result3 + sum_val + constant_expr

# Test 7: Memory management and allocation
def memory_operations():
    """Test IR for memory operations"""
    
    # Local variable allocation
    local_array = [1, 2, 3, 4, 5]
    
    # Object allocation
    obj = IRTestClass(100)
    
    # Memory access patterns
    total = 0
    for item in local_array:
        total = total + item
    
    # Object method calls (virtual dispatch if implemented)
    obj.set_value(total)
    final_result = obj.compute(2)
    
    return final_result

# Test 8: Exception handling IR (if supported)
def exception_handling_ir():
    """Test IR generation for exception handling"""
    try:
        risky_operation = 10 / 0
        return risky_operation
    except:
        safe_value = 42
        return safe_value

# Test 9: String operations IR
def string_operations_ir():
    """Test IR for string operations"""
    str1 = "Hello"
    str2 = "World"
    combined = str1 + ", " + str2 + "!"
    
    return combined

# Test 10: Floating point operations
def floating_point_ir():
    """Test IR for floating point operations"""
    pi = 3.14159
    radius = 5.0
    
    area = pi * radius * radius
    circumference = 2.0 * pi * radius
    
    return area + circumference

# Test 11: Boolean logic IR
def boolean_logic_ir(a, b, c):
    """Test IR for boolean operations"""
    result1 = a and b
    result2 = a or b
    result3 = not a
    result4 = a and (b or c)
    result5 = (a or b) and (not c)
    
    if result1 and result2:
        return result4
    elif result3:
        return result5
    else:
        return False

# Test 12: Recursive function IR
def recursive_ir(n):
    """Test IR for recursive functions"""
    if n <= 1:
        return 1
    else:
        return n * recursive_ir(n - 1)

# Test 13: Nested function definitions
def nested_functions_ir():
    """Test IR for nested function definitions"""
    
    def outer_helper(x):
        def inner_helper(y):
            return y * 2 + 5
        
        return inner_helper(x + 10)
    
    result = outer_helper(15)
    return result

# Test 14: Variable scoping and lifetime
def scoping_ir():
    """Test IR for variable scoping"""
    global_like = 100
    
    def modify_scope():
        local_var = 50
        return local_var + global_like
    
    if True:
        block_var = 25
        nested_result = modify_scope()
    
    return nested_result + block_var + global_like

# Test 15: Type coercion and conversion (if implemented)
def type_conversion_ir():
    """Test IR for type conversions"""
    int_val = 42
    float_val = 3.14
    
    # Implicit conversions
    mixed_result = int_val + float_val
    float_to_int = int(float_val)
    int_to_float = float(int_val)
    
    return mixed_result + float_to_int + int_to_float

# Test 16: Large function for code generation stress
def large_function_ir():
    """Large function to stress code generation"""
    
    # Many local variables
    v1 = 1; v2 = 2; v3 = 3; v4 = 4; v5 = 5
    v6 = 6; v7 = 7; v8 = 8; v9 = 9; v10 = 10
    v11 = 11; v12 = 12; v13 = 13; v14 = 14; v15 = 15
    
    # Complex control flow
    if v1 > v2:
        temp1 = v3 + v4
        if temp1 > v5:
            branch1 = v6 * v7
        else:
            branch1 = v8 / v9
    else:
        temp2 = v10 - v11
        if temp2 < v12:
            branch1 = v13 + v14
        else:
            branch1 = v15 * 2
    
    # Loop with many operations
    result = 0
    i = 0
    while i < 20:
        if i % 2 == 0:
            result = result + branch1 * i
        else:
            result = result - branch1 / 2
        
        # Nested computation
        temp_calc = (i * 3 + 7) % 11
        result = result + temp_calc
        
        i = i + 1
    
    return result + v1 + v2 + v3 + v4 + v5

# Test 17: Function pointer and higher-order function simulation
def higher_order_ir():
    """Test IR for higher-order function patterns"""
    
    def apply_operation(value, operation_type):
        if operation_type == 1:
            return value * 2
        elif operation_type == 2:
            return value + 10
        else:
            return value / 2
    
    def process_list(items):
        result = 0
        for item in items:
            processed = apply_operation(item, 1)
            result = result + processed
        return result
    
    test_data = [1, 2, 3, 4, 5]
    return process_list(test_data)

# Main IR test function
def run_ir_tests():
    """Run all IR generation tests"""
    
    # Basic tests
    test1 = simple_function(10)
    test2 = control_flow_ir(5)
    test3 = loop_ir()
    test4 = function_calls_ir()
    
    # Class tests
    obj = IRTestClass(25)
    test5 = obj.compute(3)
    
    # Optimization tests
    test6 = optimization_candidate()
    test7 = memory_operations()
    
    # Advanced features
    test8 = floating_point_ir()
    test9 = boolean_logic_ir(True, False, True)
    test10 = recursive_ir(5)
    test11 = nested_functions_ir()
    test12 = scoping_ir()
    
    # Stress tests
    test13 = large_function_ir()
    test14 = higher_order_ir()
    
    # String operations
    str_result = string_operations_ir()
    
    total = test1 + test2 + test3 + test4 + test5 + test6 + test7 + test8 + test10 + test11 + test12 + test13 + test14
    
    if test9:
        total = total + 1000
    
    return total

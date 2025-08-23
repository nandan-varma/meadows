# Performance and Stress Tests

# Test 1: Large function with many operations
def performance_arithmetic():
    """Test arithmetic performance with many operations"""
    result = 0
    
    # Perform 1000 arithmetic operations
    i = 0
    while i < 1000:
        result = result + i * 2 - i / 3 + i ** 2 % 7
        i = i + 1
    
    return result

# Test 2: Deep recursion test
def deep_recursion(n):
    """Test recursion depth limits"""
    if n <= 0:
        return 0
    else:
        return n + deep_recursion(n - 1)

# Test 3: Large number of variables and assignments
def many_assignments():
    """Test performance with many variable assignments"""
    a1 = 1; a2 = 2; a3 = 3; a4 = 4; a5 = 5
    a6 = 6; a7 = 7; a8 = 8; a9 = 9; a10 = 10
    a11 = 11; a12 = 12; a13 = 13; a14 = 14; a15 = 15
    a16 = 16; a17 = 17; a18 = 18; a19 = 19; a20 = 20
    a21 = 21; a22 = 22; a23 = 23; a24 = 24; a25 = 25
    a26 = 26; a27 = 27; a28 = 28; a29 = 29; a30 = 30
    a31 = 31; a32 = 32; a33 = 33; a34 = 34; a35 = 35
    a36 = 36; a37 = 37; a38 = 38; a39 = 39; a40 = 40
    a41 = 41; a42 = 42; a43 = 43; a44 = 44; a45 = 45
    a46 = 46; a47 = 47; a48 = 48; a49 = 49; a50 = 50
    
    total = a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10
    total = total + a11 + a12 + a13 + a14 + a15 + a16 + a17 + a18 + a19 + a20
    total = total + a21 + a22 + a23 + a24 + a25 + a26 + a27 + a28 + a29 + a30
    total = total + a31 + a32 + a33 + a34 + a35 + a36 + a37 + a38 + a39 + a40
    total = total + a41 + a42 + a43 + a44 + a45 + a46 + a47 + a48 + a49 + a50
    
    return total

# Test 4: Complex nested expressions
def complex_expressions():
    """Test parsing performance with complex nested expressions"""
    result1 = ((((1 + 2) * 3) - 4) / 5) + ((((6 * 7) + 8) - 9) / 10)
    result2 = (((((11 + 12) * 13) - 14) / 15) + 16) * (((((17 + 18) * 19) - 20) / 21) + 22)
    result3 = ((((((23 + 24) * 25) - 26) / 27) + 28) * 29) + ((((((30 + 31) * 32) - 33) / 34) + 35) * 36)
    
    final = (result1 + result2) * result3 - (result1 * result2) / result3
    return final

# Test 5: Large number of function calls
def function_call_overhead():
    """Test function call performance"""
    
    def small_function(x):
        return x + 1
    
    result = 0
    i = 0
    while i < 500:
        result = small_function(result)
        i = i + 1
    
    return result

# Test 6: String concatenation performance
def string_performance():
    """Test string operations performance"""
    result = ""
    i = 0
    
    while i < 100:
        result = result + "test" + str(i) + "_"
        i = i + 1
    
    return result

# Test 7: Nested loops performance
def nested_loops():
    """Test nested loop performance"""
    total = 0
    
    i = 0
    while i < 50:
        j = 0
        while j < 50:
            k = 0
            while k < 10:
                total = total + i + j + k
                k = k + 1
            j = j + 1
        i = i + 1
    
    return total

# Test 8: Large class with many methods
class LargeClass:
    """Test class with many methods for compilation performance"""
    
    def __init__(self):
        self.value = 0
    
    def method1(self): self.value = self.value + 1; return self.value
    def method2(self): self.value = self.value + 2; return self.value
    def method3(self): self.value = self.value + 3; return self.value
    def method4(self): self.value = self.value + 4; return self.value
    def method5(self): self.value = self.value + 5; return self.value
    def method6(self): self.value = self.value + 6; return self.value
    def method7(self): self.value = self.value + 7; return self.value
    def method8(self): self.value = self.value + 8; return self.value
    def method9(self): self.value = self.value + 9; return self.value
    def method10(self): self.value = self.value + 10; return self.value
    
    def method11(self): self.value = self.value * 2; return self.value
    def method12(self): self.value = self.value - 1; return self.value
    def method13(self): self.value = self.value / 2; return self.value
    def method14(self): self.value = self.value % 10; return self.value
    def method15(self): self.value = self.value ** 2; return self.value
    
    def compute_all(self):
        self.method1()
        self.method2()
        self.method3()
        self.method4()
        self.method5()
        self.method6()
        self.method7()
        self.method8()
        self.method9()
        self.method10()
        self.method11()
        self.method12()
        self.method13()
        self.method14()
        self.method15()
        return self.value

# Test 9: Memory allocation patterns
def memory_stress():
    """Test memory allocation with many objects"""
    
    # Create many class instances
    objects = []
    i = 0
    while i < 100:
        obj = LargeClass()
        obj.compute_all()
        objects.append(obj)
        i = i + 1
    
    # Process all objects
    total = 0
    for obj in objects:
        total = total + obj.value
    
    return total

# Test 10: Compilation stress test
def compilation_stress():
    """Large function that stresses the compilation pipeline"""
    
    # Variable declarations
    x1 = 1; x2 = 2; x3 = 3; x4 = 4; x5 = 5
    y1 = 10; y2 = 20; y3 = 30; y4 = 40; y5 = 50
    z1 = 100; z2 = 200; z3 = 300; z4 = 400; z5 = 500
    
    # Complex computations
    result1 = x1 * y1 + z1 - x2 * y2 + z2
    result2 = x3 * y3 + z3 - x4 * y4 + z4
    result3 = x5 * y5 + z5 - x1 * y3 + z2
    
    # Control flow
    if result1 > result2:
        if result2 > result3:
            temp = result1
            result1 = result3
            result3 = temp
        else:
            temp = result1
            result1 = result2
            result2 = temp
    else:
        if result1 > result3:
            temp = result2
            result2 = result3
            result3 = temp
        else:
            temp = result1
            result1 = result2
            result2 = result1
    
    # More complex operations
    final_result = 0
    i = 0
    while i < 20:
        if i % 2 == 0:
            final_result = final_result + result1 * i
        else:
            if i % 3 == 0:
                final_result = final_result + result2 * i
            else:
                final_result = final_result + result3 * i
        i = i + 1
    
    return final_result

# Test 11: Parser stress with deep nesting
def parser_stress():
    """Test parser with deeply nested structures"""
    
    # Deeply nested conditionals
    x = 50
    if x > 0:
        if x > 10:
            if x > 20:
                if x > 30:
                    if x > 40:
                        if x > 45:
                            result = 100
                        else:
                            result = 90
                    else:
                        result = 80
                else:
                    result = 70
            else:
                result = 60
        else:
            result = 50
    else:
        result = 0
    
    # Deeply nested arithmetic
    nested_calc = (((((x + 1) * 2) + 3) * 4) + 5) * 6
    
    return result + nested_calc

# Main performance test function
def run_performance_tests():
    """Run all performance tests"""
    
    # Arithmetic performance
    perf1 = performance_arithmetic()
    
    # Function call overhead
    perf2 = function_call_overhead()
    
    # Complex expressions
    perf3 = complex_expressions()
    
    # Nested loops
    perf4 = nested_loops()
    
    # Class operations
    large_obj = LargeClass()
    perf5 = large_obj.compute_all()
    
    # Memory stress
    perf6 = memory_stress()
    
    # Compilation stress
    perf7 = compilation_stress()
    
    # Parser stress
    perf8 = parser_stress()
    
    # Recursive performance (limited depth to avoid stack overflow)
    perf9 = deep_recursion(100)
    
    # String performance
    perf10 = len(string_performance())
    
    total_performance = perf1 + perf2 + perf3 + perf4 + perf5 + perf6 + perf7 + perf8 + perf9 + perf10
    return total_performance

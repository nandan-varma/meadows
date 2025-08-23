# Compilation and Code Generation Tests

# Test 1: Simple program that should compile to executable
def simple_main():
    x = 42
    y = x + 10
    return y

# Test 2: Function with multiple return paths
def conditional_return(n):
    if n > 0:
        return n * 2
    else:
        return 0

# Test 3: Recursive function for compilation
def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

# Test 4: Class with methods for compilation
class Calculator:
    def __init__(self):
        self.result = 0
    
    def add(self, value):
        self.result = self.result + value
        return self.result
    
    def multiply(self, value):
        self.result = self.result * value
        return self.result

# Test 5: Complex arithmetic expressions
def complex_arithmetic():
    a = 10
    b = 20
    c = 30
    
    result1 = (a + b) * c / (a - 5)
    result2 = a ** 2 + b ** 2
    result3 = (a * b + c) % 7
    
    return result1 + result2 + result3

# Test 6: String operations and manipulation
def string_operations():
    greeting = "Hello"
    name = "World"
    combined = greeting + ", " + name + "!"
    return combined

# Test 7: Boolean logic and conditions
def boolean_logic(a, b, c):
    result1 = a and b or c
    result2 = not a or (b and c)
    result3 = (a or b) and (not c)
    
    if result1 and result2:
        return result3
    else:
        return False

# Test 8: Loop constructs for compilation
def loop_operations():
    total = 0
    i = 0
    
    while i < 10:
        total = total + i
        i = i + 1
    
    return total

# Test 9: Nested function calls
def outer_function(x):
    def inner_function(y):
        return y * 2
    
    result = inner_function(x + 5)
    return result + 10

# Test 10: Array-like operations (if supported)
def list_operations():
    # Simple sequential access pattern
    values = [1, 2, 3, 4, 5]
    total = 0
    
    for value in values:
        total = total + value
    
    return total

# Test 11: Exception handling patterns (if supported)
def error_handling():
    try:
        result = 10 / 0
        return result
    except:
        return -1

# Test 12: Module-level code for compilation
module_variable = 100

def use_module_variable():
    return module_variable * 2

# Entry point function for compilation testing
def main():
    # Test basic functionality
    result1 = simple_main()
    result2 = conditional_return(5)
    result3 = fibonacci(8)
    
    # Test class instantiation
    calc = Calculator()
    calc.add(10)
    calc.multiply(3)
    
    # Test complex operations
    result4 = complex_arithmetic()
    result5 = string_operations()
    result6 = boolean_logic(True, False, True)
    result7 = loop_operations()
    result8 = outer_function(15)
    
    # Final computation
    final_result = result1 + result2 + result3 + calc.result + result7
    return final_result

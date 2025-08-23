# Test cases for function definitions

def test_simple_functions():
    # Function with no parameters
    def greet():
        print("Hello!")
    
    # Function with one parameter
    def greet_person(name):
        print("Hello, " + name)
    
    # Function with multiple parameters
    def add(a, b):
        return a + b
    
    # Function with default parameters
    def greet_with_default(name, greeting="Hello"):
        return greeting + ", " + name

def test_recursive_functions():
    # Factorial function
    def factorial(n):
        if n <= 1:
            return 1
        else:
            return n * factorial(n - 1)
    
    # Fibonacci function
    def fibonacci(n):
        if n <= 0:
            return 0
        elif n == 1:
            return 1
        else:
            return fibonacci(n - 1) + fibonacci(n - 2)
    
    # Greatest common divisor
    def gcd(a, b):
        if b == 0:
            return a
        else:
            return gcd(b, a % b)

def test_complex_functions():
    # Function with multiple statements
    def complex_calculation(x, y, z):
        temp1 = x + y
        temp2 = temp1 * z
        if temp2 > 100:
            return temp2 / 2
        else:
            return temp2 * 2
    
    # Function calling other functions
    def caller_function():
        result1 = add(10, 20)
        result2 = factorial(5)
        return result1 + result2

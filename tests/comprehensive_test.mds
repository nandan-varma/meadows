# Comprehensive test program for the Meadows language

def test_all_features():
    # Test basic data types
    integer_val = 42
    float_val = 3.14
    string_val = "Hello, World!"
    boolean_val = True
    none_val = None
    
    # Test arithmetic operations
    addition = 10 + 5
    subtraction = 10 - 5
    multiplication = 10 * 5
    division = 10 / 5
    modulo = 10 % 3
    power = 2 ** 8
    
    # Test comparison operations
    equal = 5 == 5
    not_equal = 5 != 3
    less_than = 3 < 5
    less_equal = 3 <= 5
    greater_than = 5 > 3
    greater_equal = 5 >= 3
    
    # Test logical operations
    logical_and = True and False
    logical_or = True or False
    logical_not = not True
    
    # Test control flow
    if integer_val > 0:
        print("Positive number")
    elif integer_val < 0:
        print("Negative number")
    else:
        print("Zero")
    
    # Test loops
    counter = 0
    while counter < 5:
        print("Counter:", counter)
        counter = counter + 1
    
    for i in range(3):
        print("Loop iteration:", i)

def test_functions():
    # Simple function
    def add_numbers(a, b):
        return a + b
    
    # Function with default parameter
    def greet(name, greeting="Hello"):
        return greeting + ", " + name + "!"
    
    # Recursive function
    def factorial(n):
        if n <= 1:
            return 1
        else:
            return n * factorial(n - 1)
    
    # Test function calls
    sum_result = add_numbers(10, 20)
    greeting_result = greet("Alice")
    fact_result = factorial(5)

def test_classes():
    # Define a class
    class Calculator:
        def __init__(self, initial_value):
            self.value = initial_value
        
        def add(self, x):
            self.value = self.value + x
            return self
        
        def multiply(self, x):
            self.value = self.value * x
            return self
        
        def get_result(self):
            return self.value
    
    # Use the class
    calc = Calculator(10)
    result = calc.add(5).multiply(2).get_result()
    
    # Another class example
    class Person:
        def __init__(self, name, age):
            self.name = name
            self.age = age
        
        def introduce(self):
            return "Hi, I'm " + self.name + " and I'm " + str(self.age) + " years old"
        
        def have_birthday(self):
            self.age = self.age + 1
    
    person = Person("Bob", 25)
    introduction = person.introduce()
    person.have_birthday()

def main():
    # Run all tests
    test_all_features()
    test_functions()
    test_classes()
    
    print("All tests completed!")

# Entry point
main()

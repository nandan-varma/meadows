# Example Meadows program - a simple calculator with classes

class Calculator:
    def __init__(self, name="Calculator"):
        self.name = name
        self.history = []
    
    def add(self, a, b):
        result = a + b
        self.history.append("Added " + str(a) + " + " + str(b) + " = " + str(result))
        return result
    
    def multiply(self, a, b):
        result = a * b
        self.history.append("Multiplied " + str(a) + " * " + str(b) + " = " + str(result))
        return result
    
    def print_history(self):
        print("History for " + self.name + ":")
        for entry in self.history:
            print("  " + entry)

def fibonacci(n):
    if n <= 0:
        return 0
    elif n == 1:
        return 1
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

def main():
    calc = Calculator("MyCalc")
    
    # Basic arithmetic
    sum_result = calc.add(10, 20)
    product_result = calc.multiply(sum_result, 2)
    
    print("Sum:", sum_result)
    print("Product:", product_result)
    
    # Fibonacci sequence
    print("Fibonacci numbers:")
    for i in range(10):
        fib_num = fibonacci(i)
        print("fib(" + str(i) + ") = " + str(fib_num))
    
    # Print calculation history
    calc.print_history()
    
    return 0

# Entry point
if __name__ == "main":
    main()

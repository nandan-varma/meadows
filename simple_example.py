# Simple Meadows program demonstrating the supported features

def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

def power(base, exponent):
    if exponent == 0:
        return 1
    else:
        if exponent == 1:
            return base
        else:
            return base * power(base, exponent - 1)

# Test factorial
print("Factorial of 5:", factorial(5))

# Test power function
print("2 to the power of 8:", power(2, 8))

# Test loops and conditionals
i = 0
while i < 5:
    if i % 2 == 0:
        print("Even number:", i)
    else:
        print("Odd number:", i)
    i = i + 1

# Simple test program for binary compilation

def add(a, b):
    return a + b

def multiply(x, y):
    return x * y

def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

# Main program
x = 5
y = 3
sum_result = add(x, y)
product = multiply(sum_result, 2)

print("Sum:", sum_result)
print("Product:", product)
print("Factorial of 5:", factorial(5))

# Simple loop
i = 0
while i < 3:
    print("Loop iteration:", i)
    i = i + 1

# Test cases for statement parsing

def test_assignment_statements():
    # Simple assignment
    x = 42
    name = "John"
    flag = True
    
    # Multiple assignment (if supported)
    a = b = c = 10

def test_if_statements():
    # Simple if
    if condition:
        print("true")
    
    # If-else
    if x > 0:
        print("positive")
    else:
        print("not positive")
    
    # If-elif-else
    if x > 0:
        print("positive")
    elif x < 0:
        print("negative")
    else:
        print("zero")
    
    # Nested if statements
    if outer_condition:
        if inner_condition:
            print("both true")
        else:
            print("outer true, inner false")

def test_while_loops():
    # Simple while loop
    i = 0
    while i < 10:
        print(i)
        i = i + 1
    
    # While with complex condition
    while x > 0 and y < 100:
        x = x - 1
        y = y + 2
    
    # Nested while loops
    i = 0
    while i < 3:
        j = 0
        while j < 3:
            print(i, j)
            j = j + 1
        i = i + 1

def test_for_loops():
    # Simple for loop
    for item in collection:
        print(item)
    
    # For loop with range (if supported)
    for i in range(10):
        print(i)
    
    # Nested for loops
    for i in outer_collection:
        for j in inner_collection:
            process(i, j)

def test_return_statements():
    # Simple return
    def simple_return():
        return 42
    
    # Return with expression
    def complex_return():
        return a + b * c
    
    # Conditional return
    def conditional_return(x):
        if x > 0:
            return x
        else:
            return -x

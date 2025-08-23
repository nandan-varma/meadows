# Test cases for control flow edge cases

def test_nested_control_flow():
    # Deeply nested if statements
    if True:
        if True:
            if True:
                if True:
                    result = "deeply nested"
    
    # Mixed if and while
    counter = 0
    if counter < 10:
        while counter < 5:
            if counter % 2 == 0:
                print("Even:", counter)
            counter = counter + 1

def test_early_returns():
    # Function with multiple return paths
    def classify_number(x):
        if x > 0:
            return "positive"
        elif x < 0:
            return "negative"
        else:
            return "zero"
    
    # Function with early return in loop
    def find_first_even(numbers):
        for num in numbers:
            if num % 2 == 0:
                return num
        return None

def test_complex_conditions():
    # Complex boolean expressions
    if (x > 0 and y > 0) or (x < 0 and y < 0):
        result = "same sign"
    elif x == 0 or y == 0:
        result = "contains zero"
    else:
        result = "different signs"
    
    # While with complex condition
    while (counter > 0 and flag) or (backup_counter < limit):
        if counter > 0:
            counter = counter - 1
        else:
            backup_counter = backup_counter + 1

def test_loop_edge_cases():
    # Empty loop body
    for i in range(10):
        pass
    
    # Loop with only continue
    for item in items:
        if should_skip(item):
            continue
        process(item)
    
    # Loop with break
    for item in items:
        if found_target(item):
            break
        process(item)

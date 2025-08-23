# Test cases for data types and literals

def test_integer_literals():
    # Basic integers
    zero = 0
    positive = 42
    negative = -123
    large = 999999999
    
    # Integer arithmetic
    sum_result = 10 + 20
    diff_result = 30 - 15
    prod_result = 6 * 7
    quot_result = 84 / 12

def test_float_literals():
    # Basic floats
    pi = 3.14159
    negative_float = -2.5
    zero_float = 0.0
    small_float = 0.001
    
    # Float arithmetic
    float_sum = 1.5 + 2.5
    float_prod = 2.0 * 3.0
    mixed_arithmetic = 10 + 3.5

def test_string_literals():
    # Different quote styles
    double_quoted = "Hello, World!"
    single_quoted = 'Hello, World!'
    empty_string = ""
    
    # String operations
    concatenation = "Hello, " + "World!"
    repeated = "abc" * 3
    
    # Strings with special characters
    with_spaces = "   spaced string   "
    with_numbers = "abc123def"
    with_punctuation = "Hello, World! How are you?"

def test_boolean_literals():
    # Boolean values
    true_val = True
    false_val = False
    
    # Boolean operations
    and_result = True and False
    or_result = True or False
    not_result = not True
    
    # Boolean from comparisons
    comparison_result = 5 > 3
    equality_result = "test" == "test"

def test_none_literal():
    # None value
    none_val = None
    
    # None comparisons
    is_none = none_val == None
    not_none = 42 != None

def test_type_mixing():
    # Operations between different types
    int_plus_float = 10 + 3.5
    string_times_int = "hello" * 3
    
    # Comparisons between types
    int_eq_float = 5 == 5.0
    different_types = "5" != 5

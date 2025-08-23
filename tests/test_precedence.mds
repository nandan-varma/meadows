# Test cases for operator precedence and associativity

def test_arithmetic_precedence():
    # Test basic precedence: * before +
    result1 = 2 + 3 * 4  # Should be 14, not 20
    
    # Test division and multiplication (same precedence, left associative)
    result2 = 12 / 3 * 2  # Should be 8
    
    # Test exponentiation (highest precedence, right associative)
    result3 = 2 ** 3 ** 2  # Should be 512 (2 ** (3 ** 2))
    
    # Test complex expression
    result4 = 1 + 2 * 3 ** 2 - 4 / 2  # Should be 17

def test_comparison_precedence():
    # Arithmetic before comparison
    result1 = 2 + 3 > 4  # Should be True
    
    # Comparison before logical
    result2 = 1 < 2 and 3 < 4  # Should be True
    
    # Complex logical expression
    result3 = 1 < 2 or 3 > 4 and 5 < 6  # Should be True

def test_parentheses_override():
    # Parentheses override precedence
    result1 = (2 + 3) * 4  # Should be 20
    result2 = 2 ** (3 + 2)  # Should be 32
    
    # Nested parentheses
    result3 = ((2 + 3) * 4) - (5 + 6)  # Should be 9

def test_unary_operators():
    # Unary minus has high precedence
    result1 = -2 ** 2  # Should be -4 (-(2**2))
    result2 = (-2) ** 2  # Should be 4
    
    # Unary not
    result3 = not 1 == 2  # Should be True
    result4 = not (1 == 2)  # Should be True

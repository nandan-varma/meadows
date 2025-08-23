# Test cases for class definitions

def test_simple_classes():
    # Basic class definition
    class Person:
        def __init__(self, name):
            self.name = name
        
        def greet(self):
            return "Hello, I'm " + self.name
    
    # Class with multiple methods
    class Calculator:
        def __init__(self):
            self.value = 0
        
        def add(self, x):
            self.value = self.value + x
            return self.value
        
        def subtract(self, x):
            self.value = self.value - x
            return self.value
        
        def get_value(self):
            return self.value

def test_class_with_attributes():
    # Class with instance variables
    class BankAccount:
        def __init__(self, initial_balance):
            self.balance = initial_balance
            self.transaction_count = 0
        
        def deposit(self, amount):
            self.balance = self.balance + amount
            self.transaction_count = self.transaction_count + 1
        
        def withdraw(self, amount):
            if self.balance >= amount:
                self.balance = self.balance - amount
                self.transaction_count = self.transaction_count + 1
                return True
            else:
                return False
        
        def get_balance(self):
            return self.balance

def test_complex_classes():
    # Class with complex logic
    class Stack:
        def __init__(self):
            self.items = []
            self.size = 0
        
        def push(self, item):
            self.items.append(item)
            self.size = self.size + 1
        
        def pop(self):
            if self.size > 0:
                item = self.items[self.size - 1]
                self.size = self.size - 1
                return item
            else:
                return None
        
        def peek(self):
            if self.size > 0:
                return self.items[self.size - 1]
            else:
                return None
        
        def is_empty(self):
            return self.size == 0

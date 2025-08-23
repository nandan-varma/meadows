# Integration and End-to-End Tests

# Test 1: Complete program with all language features
def complete_program_test():
    """Integration test covering all major language features"""
    
    # Global variables
    global_counter = 0
    
    # Data types
    integer_value = 42
    float_value = 3.14159
    string_value = "Integration Test"
    boolean_value = True
    none_value = None
    
    # Arithmetic operations
    arithmetic_result = integer_value + float_value * 2 - 10 / 5
    
    # String operations
    formatted_string = string_value + " - " + "Result: " + str(arithmetic_result)
    
    # Boolean logic
    logical_result = boolean_value and (arithmetic_result > 0) or False
    
    return arithmetic_result

# Test 2: Class hierarchy and inheritance (if supported)
class BaseClass:
    """Base class for inheritance testing"""
    
    def __init__(self, name):
        self.name = name
        self.value = 0
    
    def get_info(self):
        return self.name + ": " + str(self.value)
    
    def process(self):
        self.value = self.value + 10
        return self.value

class DerivedClass(BaseClass):
    """Derived class testing inheritance"""
    
    def __init__(self, name, extra):
        # Call parent constructor
        super().__init__(name)
        self.extra = extra
    
    def process(self):
        # Override parent method
        self.value = self.value + 20 + self.extra
        return self.value
    
    def specialized_method(self):
        return self.value * self.extra

# Test 3: Complex data structures and algorithms
def algorithm_test():
    """Test complex algorithms and data structures"""
    
    # Bubble sort implementation
    def bubble_sort(arr):
        n = len(arr)
        for i in range(n):
            for j in range(0, n - i - 1):
                if arr[j] > arr[j + 1]:
                    arr[j], arr[j + 1] = arr[j + 1], arr[j]
        return arr
    
    # Binary search implementation
    def binary_search(arr, target):
        left = 0
        right = len(arr) - 1
        
        while left <= right:
            mid = (left + right) // 2
            if arr[mid] == target:
                return mid
            elif arr[mid] < target:
                left = mid + 1
            else:
                right = mid - 1
        
        return -1
    
    # Test data
    test_array = [64, 34, 25, 12, 22, 11, 90]
    
    # Sort the array
    sorted_array = bubble_sort(test_array.copy())
    
    # Search for an element
    search_result = binary_search(sorted_array, 25)
    
    return len(sorted_array) + search_result

# Test 4: Mathematical computations
class MathUtils:
    """Mathematical utility class"""
    
    @staticmethod
    def factorial(n):
        if n <= 1:
            return 1
        else:
            return n * MathUtils.factorial(n - 1)
    
    @staticmethod
    def fibonacci(n):
        if n <= 1:
            return n
        else:
            return MathUtils.fibonacci(n - 1) + MathUtils.fibonacci(n - 2)
    
    @staticmethod
    def gcd(a, b):
        while b:
            a, b = b, a % b
        return a
    
    @staticmethod
    def prime_check(n):
        if n < 2:
            return False
        for i in range(2, int(n ** 0.5) + 1):
            if n % i == 0:
                return False
        return True

def math_test():
    """Test mathematical computations"""
    fact_5 = MathUtils.factorial(5)
    fib_8 = MathUtils.fibonacci(8)
    gcd_48_18 = MathUtils.gcd(48, 18)
    is_17_prime = MathUtils.prime_check(17)
    
    result = fact_5 + fib_8 + gcd_48_18
    if is_17_prime:
        result = result + 100
    
    return result

# Test 5: File I/O simulation (if supported)
def file_io_simulation():
    """Simulate file I/O operations"""
    
    # Simulate reading configuration
    config_data = {
        "app_name": "Meadows Test",
        "version": "1.0.0",
        "debug": True,
        "max_users": 1000
    }
    
    # Process configuration
    if config_data["debug"]:
        log_level = "DEBUG"
    else:
        log_level = "INFO"
    
    # Simulate writing log entries
    log_entries = []
    for i in range(5):
        entry = log_level + ": Operation " + str(i) + " completed"
        log_entries.append(entry)
    
    return len(log_entries) + config_data["max_users"]

# Test 6: Event system simulation
class EventSystem:
    """Simple event system for testing"""
    
    def __init__(self):
        self.listeners = []
        self.event_count = 0
    
    def add_listener(self, listener):
        self.listeners.append(listener)
    
    def emit_event(self, event_data):
        self.event_count = self.event_count + 1
        
        for listener in self.listeners:
            listener.handle_event(event_data)
    
    def get_stats(self):
        return {
            "total_events": self.event_count,
            "listener_count": len(self.listeners)
        }

class EventListener:
    """Event listener for testing"""
    
    def __init__(self, name):
        self.name = name
        self.handled_events = 0
    
    def handle_event(self, event_data):
        self.handled_events = self.handled_events + 1
        # Process event data
        processed = event_data * 2 + 10
        return processed

def event_system_test():
    """Test event system implementation"""
    system = EventSystem()
    
    listener1 = EventListener("Listener1")
    listener2 = EventListener("Listener2")
    
    system.add_listener(listener1)
    system.add_listener(listener2)
    
    # Emit several events
    for i in range(10):
        system.emit_event(i + 1)
    
    stats = system.get_stats()
    total_handled = listener1.handled_events + listener2.handled_events
    
    return stats["total_events"] + total_handled

# Test 7: State machine implementation
class StateMachine:
    """Simple state machine for testing"""
    
    def __init__(self):
        self.state = "IDLE"
        self.transitions = 0
    
    def transition_to(self, new_state):
        valid_transitions = {
            "IDLE": ["RUNNING", "ERROR"],
            "RUNNING": ["IDLE", "PAUSED", "ERROR"],
            "PAUSED": ["RUNNING", "IDLE"],
            "ERROR": ["IDLE"]
        }
        
        if new_state in valid_transitions.get(self.state, []):
            self.state = new_state
            self.transitions = self.transitions + 1
            return True
        else:
            return False
    
    def get_state(self):
        return self.state
    
    def get_transition_count(self):
        return self.transitions

def state_machine_test():
    """Test state machine implementation"""
    sm = StateMachine()
    
    # Test valid transitions
    sm.transition_to("RUNNING")
    sm.transition_to("PAUSED")
    sm.transition_to("RUNNING")
    sm.transition_to("ERROR")
    sm.transition_to("IDLE")
    
    # Test invalid transition (should fail)
    invalid_result = sm.transition_to("INVALID_STATE")
    
    result = sm.get_transition_count()
    if not invalid_result:
        result = result + 10  # Bonus for proper error handling
    
    return result

# Test 8: Data processing pipeline
def data_processing_test():
    """Test data processing pipeline"""
    
    # Generate test data
    raw_data = []
    for i in range(20):
        raw_data.append({
            "id": i,
            "value": i * 2 + 5,
            "category": "A" if i % 2 == 0 else "B"
        })
    
    # Filter data
    filtered_data = []
    for item in raw_data:
        if item["value"] > 10:
            filtered_data.append(item)
    
    # Transform data
    transformed_data = []
    for item in filtered_data:
        transformed_item = {
            "id": item["id"],
            "processed_value": item["value"] * 1.5,
            "category": item["category"]
        }
        transformed_data.append(transformed_item)
    
    # Aggregate data
    category_totals = {"A": 0, "B": 0}
    for item in transformed_data:
        category_totals[item["category"]] = category_totals[item["category"]] + item["processed_value"]
    
    return int(category_totals["A"] + category_totals["B"])

# Test 9: Cache implementation
class SimpleCache:
    """Simple cache implementation for testing"""
    
    def __init__(self, max_size):
        self.max_size = max_size
        self.cache = {}
        self.access_order = []
    
    def get(self, key):
        if key in self.cache:
            # Move to end (most recently used)
            self.access_order.remove(key)
            self.access_order.append(key)
            return self.cache[key]
        else:
            return None
    
    def put(self, key, value):
        if key in self.cache:
            # Update existing
            self.cache[key] = value
            self.access_order.remove(key)
            self.access_order.append(key)
        else:
            # Add new
            if len(self.cache) >= self.max_size:
                # Remove least recently used
                lru_key = self.access_order.pop(0)
                del self.cache[lru_key]
            
            self.cache[key] = value
            self.access_order.append(key)
    
    def size(self):
        return len(self.cache)

def cache_test():
    """Test cache implementation"""
    cache = SimpleCache(3)
    
    # Add items
    cache.put("a", 1)
    cache.put("b", 2)
    cache.put("c", 3)
    
    # Access items
    val_a = cache.get("a")
    val_b = cache.get("b")
    
    # Add another item (should evict "c")
    cache.put("d", 4)
    
    # Try to get evicted item
    val_c = cache.get("c")  # Should be None
    
    result = cache.size() + (val_a or 0) + (val_b or 0)
    if val_c is None:
        result = result + 10  # Bonus for correct eviction
    
    return result

# Test 10: Main integration test
def main_integration_test():
    """Main integration test combining all features"""
    
    # Test basic functionality
    basic_result = complete_program_test()
    
    # Test object-oriented features
    base_obj = BaseClass("Base")
    derived_obj = DerivedClass("Derived", 5)
    
    base_processed = base_obj.process()
    derived_processed = derived_obj.process()
    specialized = derived_obj.specialized_method()
    
    # Test algorithms
    algo_result = algorithm_test()
    
    # Test mathematical functions
    math_result = math_test()
    
    # Test file I/O simulation
    io_result = file_io_simulation()
    
    # Test event system
    event_result = event_system_test()
    
    # Test state machine
    state_result = state_machine_test()
    
    # Test data processing
    data_result = data_processing_test()
    
    # Test cache
    cache_result = cache_test()
    
    # Combine all results
    total_result = (basic_result + base_processed + derived_processed + 
                   specialized + algo_result + math_result + io_result + 
                   event_result + state_result + data_result + cache_result)
    
    return int(total_result)

# Entry point for integration testing
if __name__ == "__main__":
    final_result = main_integration_test()
    print("Integration test completed with result:", final_result)

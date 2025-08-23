# Real-world Application Examples

# Test 1: Simple calculator application
class Calculator:
    """Simple calculator with memory and operations history"""
    
    def __init__(self):
        self.memory = 0
        self.history = []
        self.last_result = 0
    
    def add(self, a, b):
        result = a + b
        self.last_result = result
        self.history.append(f"ADD {a} + {b} = {result}")
        return result
    
    def subtract(self, a, b):
        result = a - b
        self.last_result = result
        self.history.append(f"SUB {a} - {b} = {result}")
        return result
    
    def multiply(self, a, b):
        result = a * b
        self.last_result = result
        self.history.append(f"MUL {a} * {b} = {result}")
        return result
    
    def divide(self, a, b):
        if b == 0:
            return None  # Error case
        result = a / b
        self.last_result = result
        self.history.append(f"DIV {a} / {b} = {result}")
        return result
    
    def power(self, a, b):
        result = a ** b
        self.last_result = result
        self.history.append(f"POW {a} ^ {b} = {result}")
        return result
    
    def store_memory(self):
        self.memory = self.last_result
        self.history.append(f"STORE {self.last_result} -> memory")
    
    def recall_memory(self):
        self.history.append(f"RECALL memory = {self.memory}")
        return self.memory
    
    def clear_memory(self):
        self.memory = 0
        self.history.append("CLEAR memory")
    
    def get_history(self):
        return self.history
    
    def clear_history(self):
        self.history = []

def calculator_test():
    """Test calculator application"""
    calc = Calculator()
    
    # Perform calculations
    result1 = calc.add(10, 5)
    result2 = calc.multiply(result1, 3)
    calc.store_memory()
    
    result3 = calc.divide(100, 4)
    result4 = calc.subtract(result3, calc.recall_memory())
    
    result5 = calc.power(2, 8)
    
    # Final calculation
    final = calc.add(result4, result5)
    
    return int(final)

# Test 2: Banking system simulation
class BankAccount:
    """Simple bank account with transaction history"""
    
    def __init__(self, account_number, initial_balance):
        self.account_number = account_number
        self.balance = initial_balance
        self.transactions = []
        self.transaction_id = 1000
    
    def deposit(self, amount):
        if amount > 0:
            self.balance = self.balance + amount
            self.transactions.append({
                "id": self.transaction_id,
                "type": "DEPOSIT",
                "amount": amount,
                "balance": self.balance
            })
            self.transaction_id = self.transaction_id + 1
            return True
        return False
    
    def withdraw(self, amount):
        if amount > 0 and amount <= self.balance:
            self.balance = self.balance - amount
            self.transactions.append({
                "id": self.transaction_id,
                "type": "WITHDRAWAL",
                "amount": amount,
                "balance": self.balance
            })
            self.transaction_id = self.transaction_id + 1
            return True
        return False
    
    def transfer(self, target_account, amount):
        if self.withdraw(amount):
            target_account.deposit(amount)
            # Update transaction type
            last_transaction = self.transactions[-1]
            last_transaction["type"] = "TRANSFER_OUT"
            last_transaction["target"] = target_account.account_number
            return True
        return False
    
    def get_balance(self):
        return self.balance
    
    def get_statement(self):
        return self.transactions

class Bank:
    """Bank management system"""
    
    def __init__(self):
        self.accounts = {}
        self.next_account_number = 1001
    
    def create_account(self, initial_balance):
        account_number = self.next_account_number
        self.next_account_number = self.next_account_number + 1
        
        account = BankAccount(account_number, initial_balance)
        self.accounts[account_number] = account
        
        return account_number
    
    def get_account(self, account_number):
        return self.accounts.get(account_number)
    
    def total_deposits(self):
        total = 0
        for account in self.accounts.values():
            total = total + account.get_balance()
        return total

def banking_test():
    """Test banking system"""
    bank = Bank()
    
    # Create accounts
    acc1_num = bank.create_account(1000)
    acc2_num = bank.create_account(500)
    acc3_num = bank.create_account(2000)
    
    # Get account objects
    acc1 = bank.get_account(acc1_num)
    acc2 = bank.get_account(acc2_num)
    acc3 = bank.get_account(acc3_num)
    
    # Perform transactions
    acc1.deposit(250)
    acc2.withdraw(100)
    acc1.transfer(acc3, 300)
    acc3.deposit(500)
    acc2.deposit(200)
    
    # Calculate final totals
    total_balance = bank.total_deposits()
    
    return int(total_balance)

# Test 3: Inventory management system
class Product:
    """Product in inventory system"""
    
    def __init__(self, product_id, name, price, quantity):
        self.product_id = product_id
        self.name = name
        self.price = price
        self.quantity = quantity
    
    def update_quantity(self, new_quantity):
        self.quantity = new_quantity
    
    def adjust_quantity(self, adjustment):
        self.quantity = self.quantity + adjustment
        if self.quantity < 0:
            self.quantity = 0
    
    def get_value(self):
        return self.price * self.quantity

class InventorySystem:
    """Inventory management system"""
    
    def __init__(self):
        self.products = {}
        self.next_product_id = 1
    
    def add_product(self, name, price, quantity):
        product_id = self.next_product_id
        self.next_product_id = self.next_product_id + 1
        
        product = Product(product_id, name, price, quantity)
        self.products[product_id] = product
        
        return product_id
    
    def update_stock(self, product_id, quantity_change):
        if product_id in self.products:
            self.products[product_id].adjust_quantity(quantity_change)
            return True
        return False
    
    def get_product(self, product_id):
        return self.products.get(product_id)
    
    def get_total_value(self):
        total = 0
        for product in self.products.values():
            total = total + product.get_value()
        return total
    
    def get_low_stock_products(self, threshold):
        low_stock = []
        for product in self.products.values():
            if product.quantity <= threshold:
                low_stock.append(product)
        return low_stock
    
    def search_by_name(self, name):
        results = []
        for product in self.products.values():
            if name in product.name:
                results.append(product)
        return results

def inventory_test():
    """Test inventory management system"""
    inventory = InventorySystem()
    
    # Add products
    prod1 = inventory.add_product("Laptop", 999.99, 50)
    prod2 = inventory.add_product("Mouse", 29.99, 200)
    prod3 = inventory.add_product("Keyboard", 79.99, 75)
    prod4 = inventory.add_product("Monitor", 299.99, 30)
    
    # Update stock
    inventory.update_stock(prod1, -10)  # Sold 10 laptops
    inventory.update_stock(prod2, 50)   # Received 50 mice
    inventory.update_stock(prod3, -25)  # Sold 25 keyboards
    
    # Get total value
    total_value = inventory.get_total_value()
    
    # Check low stock
    low_stock = inventory.get_low_stock_products(40)
    low_stock_count = len(low_stock)
    
    return int(total_value + low_stock_count * 1000)

# Test 4: Student grade management system
class Student:
    """Student with grades"""
    
    def __init__(self, student_id, name):
        self.student_id = student_id
        self.name = name
        self.grades = {}
        self.total_credits = 0
    
    def add_grade(self, subject, grade, credits):
        self.grades[subject] = {"grade": grade, "credits": credits}
        self.total_credits = self.total_credits + credits
    
    def get_gpa(self):
        if self.total_credits == 0:
            return 0.0
        
        total_points = 0
        for grade_info in self.grades.values():
            total_points = total_points + grade_info["grade"] * grade_info["credits"]
        
        return total_points / self.total_credits
    
    def get_grade_in_subject(self, subject):
        return self.grades.get(subject, {}).get("grade", 0)

class GradeManager:
    """Grade management system"""
    
    def __init__(self):
        self.students = {}
        self.next_student_id = 1001
    
    def add_student(self, name):
        student_id = self.next_student_id
        self.next_student_id = self.next_student_id + 1
        
        student = Student(student_id, name)
        self.students[student_id] = student
        
        return student_id
    
    def get_student(self, student_id):
        return self.students.get(student_id)
    
    def calculate_class_average(self, subject):
        total_grade = 0
        count = 0
        
        for student in self.students.values():
            grade = student.get_grade_in_subject(subject)
            if grade > 0:
                total_grade = total_grade + grade
                count = count + 1
        
        if count > 0:
            return total_grade / count
        return 0.0
    
    def get_honor_students(self, min_gpa):
        honor_students = []
        for student in self.students.values():
            if student.get_gpa() >= min_gpa:
                honor_students.append(student)
        return honor_students

def grade_management_test():
    """Test grade management system"""
    gm = GradeManager()
    
    # Add students
    student1_id = gm.add_student("Alice Johnson")
    student2_id = gm.add_student("Bob Smith")
    student3_id = gm.add_student("Carol Brown")
    
    # Get student objects
    student1 = gm.get_student(student1_id)
    student2 = gm.get_student(student2_id)
    student3 = gm.get_student(student3_id)
    
    # Add grades
    student1.add_grade("Math", 3.8, 4)
    student1.add_grade("Physics", 3.6, 3)
    student1.add_grade("Chemistry", 3.9, 3)
    
    student2.add_grade("Math", 3.2, 4)
    student2.add_grade("Physics", 3.4, 3)
    student2.add_grade("Chemistry", 3.1, 3)
    
    student3.add_grade("Math", 3.9, 4)
    student3.add_grade("Physics", 3.8, 3)
    student3.add_grade("Chemistry", 4.0, 3)
    
    # Calculate metrics
    math_average = gm.calculate_class_average("Math")
    honor_students = gm.get_honor_students(3.5)
    
    # Calculate result
    total_gpa = student1.get_gpa() + student2.get_gpa() + student3.get_gpa()
    result = int((total_gpa + math_average + len(honor_students)) * 100)
    
    return result

# Test 5: Task management system
class Task:
    """Task in the management system"""
    
    def __init__(self, task_id, title, priority, estimated_hours):
        self.task_id = task_id
        self.title = title
        self.priority = priority  # 1=High, 2=Medium, 3=Low
        self.estimated_hours = estimated_hours
        self.actual_hours = 0
        self.status = "TODO"  # TODO, IN_PROGRESS, DONE
        self.dependencies = []
    
    def start_task(self):
        if self.status == "TODO":
            self.status = "IN_PROGRESS"
            return True
        return False
    
    def complete_task(self, actual_hours):
        if self.status == "IN_PROGRESS":
            self.status = "DONE"
            self.actual_hours = actual_hours
            return True
        return False
    
    def add_dependency(self, task_id):
        self.dependencies.append(task_id)
    
    def can_start(self, completed_tasks):
        for dep_id in self.dependencies:
            if dep_id not in completed_tasks:
                return False
        return True

class TaskManager:
    """Task management system"""
    
    def __init__(self):
        self.tasks = {}
        self.next_task_id = 1
    
    def create_task(self, title, priority, estimated_hours):
        task_id = self.next_task_id
        self.next_task_id = self.next_task_id + 1
        
        task = Task(task_id, title, priority, estimated_hours)
        self.tasks[task_id] = task
        
        return task_id
    
    def get_task(self, task_id):
        return self.tasks.get(task_id)
    
    def get_available_tasks(self):
        completed_tasks = []
        for task in self.tasks.values():
            if task.status == "DONE":
                completed_tasks.append(task.task_id)
        
        available = []
        for task in self.tasks.values():
            if task.status == "TODO" and task.can_start(completed_tasks):
                available.append(task)
        
        return available
    
    def get_project_stats(self):
        total_estimated = 0
        total_actual = 0
        completed_count = 0
        
        for task in self.tasks.values():
            total_estimated = total_estimated + task.estimated_hours
            if task.status == "DONE":
                total_actual = total_actual + task.actual_hours
                completed_count = completed_count + 1
        
        return {
            "total_tasks": len(self.tasks),
            "completed_tasks": completed_count,
            "total_estimated_hours": total_estimated,
            "total_actual_hours": total_actual
        }

def task_management_test():
    """Test task management system"""
    tm = TaskManager()
    
    # Create tasks
    task1 = tm.create_task("Design Database", 1, 8)
    task2 = tm.create_task("Implement API", 1, 16)
    task3 = tm.create_task("Write Tests", 2, 6)
    task4 = tm.create_task("Deploy Application", 2, 4)
    
    # Set dependencies
    task2_obj = tm.get_task(task2)
    task3_obj = tm.get_task(task3)
    task4_obj = tm.get_task(task4)
    
    task2_obj.add_dependency(task1)  # API depends on Database
    task3_obj.add_dependency(task2)  # Tests depend on API
    task4_obj.add_dependency(task3)  # Deploy depends on Tests
    
    # Complete tasks
    task1_obj = tm.get_task(task1)
    task1_obj.start_task()
    task1_obj.complete_task(10)  # Took longer than estimated
    
    task2_obj.start_task()
    task2_obj.complete_task(14)  # Under estimate
    
    task3_obj.start_task()
    task3_obj.complete_task(8)   # Over estimate
    
    # Get project statistics
    stats = tm.get_project_stats()
    available_tasks = tm.get_available_tasks()
    
    result = (stats["completed_tasks"] * 100 + 
              stats["total_actual_hours"] * 10 + 
              len(available_tasks) * 50)
    
    return result

# Main test function for real-world applications
def run_application_tests():
    """Run all real-world application tests"""
    
    # Test calculator
    calc_result = calculator_test()
    
    # Test banking system
    bank_result = banking_test()
    
    # Test inventory system
    inventory_result = inventory_test()
    
    # Test grade management
    grade_result = grade_management_test()
    
    # Test task management
    task_result = task_management_test()
    
    # Combine results
    total_result = calc_result + bank_result + inventory_result + grade_result + task_result
    
    return total_result

# Entry point for application testing
def main():
    """Main entry point for real-world application tests"""
    result = run_application_tests()
    return result

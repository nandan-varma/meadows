# Meadows Language Reference

## Overview
Meadows is a simple, imperative programming language with C-like syntax. It compiles to native executables via LLVM.

## File Extension
Source files must have the `.ms` extension.

## Comments
```meadows
# This is a single-line comment
// This is also a single-line comment
```

## Types

### Integer (i32)
- 32-bit signed integer
- Range: -2,147,483,648 to 2,147,483,647
- Literals: `0`, `42`, `-17`

### String
- Pointer type to null-terminated character array
- Literals: `"hello"`, `"world\n"`
- Escape sequences: `\n`, `\t`, `\\`, `\"`, `\r`, `\0`, `\b`, `\f`

### Array
- Fixed-size, compile-time constant
- Element type: i32 only
- Literals: `[1, 2, 3]`

### Object
- Struct-like composite type
- Field types inferred from values
- Literals: `{name: "test", value: 42}`

## Variables

### Declaration
```meadows
let x = 42;
let message = "hello";
let numbers = [1, 2, 3];
```

### Assignment
```meadows
x = 100;
message = "updated";
```

## Operators

### Arithmetic
```meadows
+   # Addition
-   # Subtraction
*   # Multiplication
/   # Division (with runtime validation)
```

### Comparison
```meadows
==  # Equal
!=  # Not equal
>   # Greater than
<   # Less than
>=  # Greater than or equal
<=  # Less than or equal
```

### Logical
```meadows
&&  # Logical AND
||  # Logical OR
!   # Logical NOT
```

### String Concatenation
```meadows
"hello" + " " + "world"  # "hello world"
```

## Control Flow

### If Statement
```meadows
if (condition) {
  # then branch
} else {
  # else branch
}
```

### While Loop
```meadows
while (condition) {
  # body
}
```

### For Loop (range-based)
```meadows
for (i in range(0, 10)) {
  # body
}
```

## Functions

### Declaration
```meadows
func greet(name) {
  print("Hello, " + name);
}
```

### Return Statement
```meadows
func square(x) {
  return x * x;
}
```

## Built-in Functions

### print
```meadows
print(value);  # Prints with newline
```

### range
```meadows
range(start, end);  # Returns iterator from start to end
```

## Standard Library

### std.args - Command-Line Arguments

Import the args module to access command-line arguments:

```meadows
import std.args;

func main() -> i32 {
    let count = args();        # Get number of arguments
    let prog = getarg(0);     # Get program name
    let arg1 = getarg(1);     # Get first argument
    print(count);
    return 0;
}
```

#### args()
Returns the number of command-line arguments (including the program name).

#### getarg(n)
Returns the nth command-line argument as a string. Argument 0 is always the program name.

**Example:**
```bash
$ ./program.out hello world
# args() returns 3
# getarg(0) returns "./program.out"
# getarg(1) returns "hello"
# getarg(2) returns "world"
```

## Directory Operations

### opendir(path)
Opens a directory and returns a handle (i64) for use with readdir and closedir.

```meadows
import std.dir;

let dir = opendir(".");  # Open current directory
if (dir == 0) {
    print("Could not open directory\n");
    return 1;
}
```

### readdir(dir_ptr)
Reads the next entry from an open directory. Returns empty string ("") when no more entries.

```meadows
import std.dir;

let entry = readdir(dir);
if (strcmp(entry, "") == 0) {
    # No more entries
} else {
    print(entry);  # Print filename
}
```

### closedir(dir_ptr)
Closes an open directory handle.

```meadows
import std.dir;

closedir(dir);
```

### mkdir(path, mode)
Creates a new directory with the specified permissions (mode).

```meadows
import std.dir;

mkdir("new_directory", 0755);
```

### rmdir(path)
Removes an empty directory.

```meadows
import std.dir;

rmdir("old_directory");
```

### is_directory(path)
Returns 1 if the path is a directory, 0 otherwise.

```meadows
import std.dir;

if (is_directory("mydir") == 1) {
    print("Is a directory\n");
}
```

## Statements

All statements end with `;`:
```meadows
let x = 5;
print(x);
return x;
```

## Expressions

### Primary Expressions
```meadows
42              # Numeric literal
"hello"         # String literal
[1, 2, 3]       # Array literal
{key: value}    # Object literal
identifier      # Variable reference
```

### Binary Expressions
```meadows
x + y           # Arithmetic
a == b          # Comparison
p && q          # Logical
str1 + str2     # String concatenation
```

### Unary Expressions
```meadows
-x              # Negation
!flag           # Logical NOT
```

### Index and Field Access
```meadows
arr[0]          # Array indexing
obj.field       # Field access
```

### Function Call
```meadows
func_name(arg1, arg2)
```

## Runtime Errors

### Division by Zero
Validation check before division. Prints error and exits with code -1.

### Array Index Out of Bounds
Validation check on array access. Prints error and exits with code -1.

## Limitations

- No dynamic memory allocation (malloc available but not recommended)
- Arrays and objects are compile-time constants only
- Single return type (i32) for functions
- No recursion support
- No type inference (all variables inferred from literals)

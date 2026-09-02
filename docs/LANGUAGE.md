# Meadows Language Reference

## Overview

Meadows is a small, imperative programming language with C-like syntax. It
compiles to native executables via LLVM 17.

## File Extension

Source files must have the `.ms` extension.

## Comments

```meadows
# Hash-style single-line comment
// Slash-style single-line comment
```

## Types

Meadows is dynamically resolved at the AST level — variables don't carry
explicit type annotations. The three kinds of values are:

### Integer (i32)

- 32-bit signed integer
- Range: -2,147,483,648 to 2,147,483,647
- Literals: `0`, `42`, `-17`

### String

- Pointer to a null-terminated byte array
- Literals: `"hello"`, `"line\n"`
- Escape sequences: `\n`, `\t`, `\\`, `\"`, `\r`, `\0`, `\b`, `\f`

### Array

- Fixed-size, allocated at the declaration site
- Element type: i32 only
- Literals: `[1, 2, 3]`

### Object

- Struct-like composite with named fields
- Literals: `{name: "test", value: 42}`
- Field access: `obj.fieldName`

## Variables

```meadows
let x = 42;
let message = "hello";
let numbers = [1, 2, 3];
let person = {name: "Alice", age: 30};

x = 100;             # Reassignment
message = "updated";
numbers[0] = 99;     # Array element assignment
person.age = 31;     # Object field assignment
```

A variable must be declared before use. Re-declaring the same name in the same
scope is an error (`E3003`). Declaring the same name in an inner scope is a
warning (`W6004`, shadowing).

A valid assignment target is a plain variable, an array element (`arr[i] =
...`), or an object field (`obj.field = ...`) — anything else is a compile-time
error (`E2010 PARSE_INVALID_ASSIGNMENT_TARGET`). All three evaluate to the
assigned value, so `print(arr[0] = 5);` is valid.

## Operators

### Arithmetic

| Op | Meaning                                |
|----|----------------------------------------|
| `+` | Addition (integers) / concatenation (strings) |
| `-` | Subtraction                           |
| `*` | Multiplication                        |
| `/` | Integer division (runtime div-by-zero check) |
| `%` | Modulo (runtime div-by-zero check)    |

### Comparison

`==`, `!=`, `>`, `<`, `>=`, `<=`

### Logical

`&&` (and), `||` (or), `!` (not)

### Unary

`-x` (negate), `!x` (logical not)

### String concatenation

```meadows
"hello" + " " + "world"   # "hello world"
```

Concatenation with a compile-time-constant integer is permitted; non-constant
integers must be converted with `str()` first.

## Control flow

### If

```meadows
if (condition) {
  ...
} else {
  ...
}
```

The `else` branch is optional.

### While

```meadows
while (condition) {
  ...
}
```

### For (range-based)

`range` is **syntax**, not a callable function. It is only valid in `for`:

```meadows
for (i in range(0, 10)) {
  print(i);
}
```

The loop variable `i` takes values `start`, `start+1`, …, `end-1`.

### Break and continue

```meadows
while (1 == 1) {
  if (done) { break; }
  if (skip) { continue; }
  ...
}
```

`break` and `continue` outside a loop are errors (`E3009`, `E3010`).

## Functions

```meadows
func greet(name) {
  print("Hello, " + name);
}

func square(x) {
  return x * x;
}
```

- All parameters are i32 currently. Passing a string to a user-defined function
  is **not** supported (LLVM IR verification will fail).
- Return type is i32. `return;` (no expression) is allowed; it returns 0.
- Functions can call other functions defined later in the same file (mutual /
  forward calls are resolved in a pre-pass).
- Recursion is supported.
- Re-defining a function is an error (`E3004`).

## Built-in functions

| Function       | Description                                              |
|----------------|----------------------------------------------------------|
| `print(value)` | Print an integer or string, followed by a newline        |
| `len(s)`       | Length of a string, or of an array (i32)                 |
| `str(n)`       | Format an integer as a decimal string (heap-allocated)   |

Built-ins are validated by the semantic analyzer; calling them with the wrong
arg count is an error (`E3006`).

## Statements

Every statement ends with `;`:

```meadows
let x = 5;
print(x);
return x;
```

A block (`{ ... }`) is a sequence of statements with its own scope.

## Expressions

### Primary

```meadows
42              # Integer literal
"hello"         # String literal
[1, 2, 3]       # Array literal
{a: 1, b: 2}    # Object literal
identifier      # Variable reference
(expr)          # Parenthesized
```

### Index and field access

```meadows
arr[0]
obj.field
```

Indexing an array out of bounds is a **runtime** error that prints a message
and exits with code -1. Accessing an unknown field is a compile-time error —
`E3012` when the object is an inline literal at the access site, or a
CodeGen error when it's a variable declared directly from an object literal
(`let o = {a: 1}; let p = o;` — `p` carries `o`'s shape too). Field access
through anything else (a chained access like `a.b.c`, or a variable whose
initializer isn't traceable to a literal) is not supported and is a
compile-time error, not silently wrong output.

### Function call

```meadows
func_name(arg1, arg2)
```

## Compile-time diagnostics

| Code  | Name                        |
|-------|-----------------------------|
| E1001 | LEX_UNTERMINATED_STRING     |
| E1002 | LEX_INVALID_CHARACTER       |
| E2001 | PARSE_EXPECTED_SEMICOLON    |
| E2002 | PARSE_EXPECTED_EXPRESSION   |
| E2010 | PARSE_INVALID_ASSIGNMENT_TARGET |
| E3001 | SEM_UNDEFINED_VARIABLE      |
| E3002 | SEM_UNDEFINED_FUNCTION      |
| E3003 | SEM_REDEFINED_VARIABLE      |
| E3004 | SEM_REDEFINED_FUNCTION      |
| E3006 | SEM_INVALID_ARGUMENT_COUNT  |
| E3009 | SEM_BREAK_OUTSIDE_LOOP      |
| E3010 | SEM_CONTINUE_OUTSIDE_LOOP   |
| E3011 | SEM_RETURN_OUTSIDE_FUNCTION |
| E3012 | SEM_UNKNOWN_FIELD           |
| W6001 | WARN_UNUSED_VARIABLE        |
| W6003 | WARN_UNREACHABLE_CODE       |
| W6004 | WARN_SHADOWING_VARIABLE     |

Warnings can be controlled with `-Wall`, `-Wextra`, `-Werror`, or `-Wno-<name>`
(e.g. `-Wno-unused-variable`).

## Runtime errors

These are checked at runtime; on failure the program prints a message and
exits with code -1:

- Division by zero (`/`, `%`)
- Array index out of bounds

## Current limitations

These apply to the **native LLVM backend** (`meadows` CLI → `.ll` → `clang++`):

- Array and string elements are i32-only at the value level
- Function parameters are i32-only — strings cannot be passed to user functions
- No floating-point type
- No dynamic resizable arrays
- No imports / multi-file modules
- No standard library beyond `print`, `len`, `str`
- Field access requires the object be an inline literal, or a variable
  declared directly from one (including through a chain of `let b = a;`
  aliases) — chained access (`a.b.c`) and anything else isn't supported

The [browser playground](https://meadows.nandanvarma.com) runs a separate
interpreter backend (`src/interpreter/`) that does not carry the i32-only
restrictions above, since none of them are inherent to the language as
parsed and semantically analyzed — they're specific to generating i32-typed
LLVM IR. A program that only compiles with the interpreter (e.g. one that
passes a string to a user function) will not yet build with the native
`meadows` compiler.

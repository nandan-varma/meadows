# Meadows language reference

Meadows is a small, dynamically typed, imperative language with C-like syntax.
The native compiler, the interpreter and the browser playground implement
exactly the same language: every program in `tests/conformance` must produce
identical output, error messages and exit status on all of them.

## Source files

Source files use the `.ms` extension and are UTF-8. Strings are byte strings:
`len("é")` is `2`. Comments start with `#` or `//` and run to the end of the
line. Whitespace is insignificant. Identifiers match `[A-Za-z_][A-Za-z0-9_]*`.

Keywords: `let func if else for while return in range break continue import true
false`. `true` and `false` are the integers `1` and `0`.

## Values

| Type       | Notes                                                                 |
|------------|-----------------------------------------------------------------------|
| `integer`  | 32-bit signed. Arithmetic wraps on overflow.                           |
| `float`    | 64-bit IEEE 754. Literals need digits on both sides of the point.      |
| `string`   | Immutable byte string, at most 1 MiB. May contain `\0`.                |
| `array`    | Ordered, mutable, heterogeneous. At most 4,194,304 elements.           |
| `object`   | Mutable record with a fixed set of named fields, kept in source order. |
| `function` | A reference to a top-level function.                                   |

Arrays, objects and functions are references: `let b = a;` makes both names
refer to the same value. Reference cycles are never reclaimed before the program
exits.

Integer literals range from `-2147483648` to `2147483647`; larger literals are
a compile error. String escapes are `\n \t \r \0 \b \f \\ \"`; any other escape
is a compile error. A string literal cannot span lines.

## Variables and scope

```meadows
let x = 1;
x = x + 1;
```

A variable must be declared before use and cannot be declared twice in one
scope (`E3003`). Blocks, `if`/`else` branches and loop bodies have their own
scope. Shadowing an outer variable is allowed and produces warning `W6004`.

Functions are closed: a function body sees only its parameters, its own locals
and other functions. It cannot read or write top-level variables. Pass values in
as arguments and return results.

A name has one meaning. A variable cannot share a name with a function or a
built-in (`E3013`), and a function name cannot be assigned to (`E3015`).

## Expressions

Operators from lowest to highest precedence:

| Operators                  | Notes                                  |
|----------------------------|----------------------------------------|
| `=`                        | Assignment, right-associative          |
| `\|\|`                     | Short-circuit or                       |
| `&&`                       | Short-circuit and                      |
| `==` `!=`                  | Equality                               |
| `<` `<=` `>` `>=`          | Ordering, numbers only                 |
| `+` `-`                    | Addition, subtraction, concatenation   |
| `*` `/` `%`                | Multiplication, division, remainder    |
| `-` `!`                    | Unary                                  |
| `()` `[]` `.`              | Call, index, field; chain freely       |

Operands are evaluated left to right. The operation itself, and any error it
raises, happens after its operands are evaluated.

### Arithmetic

Two integers give an integer, using truncating division. `INT_MIN / -1` is
`INT_MIN` and `INT_MIN % -1` is `0`. If either operand is a float the other is
promoted and the result is a float; `%` on floats is `fmod`. Dividing by zero,
integer or float, is a runtime error.

`+` concatenates when either operand is a string and the other is a string or a
number. Numbers are formatted as `%d` (integers) and `%g` (floats).

### Comparison and equality

`==` and `!=` accept any operands and never fail. Numbers compare by value
(`1 == 1.0`), strings by content, and arrays, objects and functions by
identity. Different types are never equal. `<`, `<=`, `>`, `>=` accept numbers
only.

### Logic

`&&`, `||` and `!` require integer operands, short-circuit, and produce `0` or
`1`. Conditions of `if` and `while` must be integers. Comparisons already
produce integers; write `x > 0.0` rather than testing a float directly.

### Indexing and fields

`a[i]` requires an array and an integer index within bounds. `o.name` requires
an object that has the field. Both can be assigned, and an assignment evaluates
to the assigned value: `print(a[0] = 5);`. A field cannot be added to an
existing object. Accessing an unknown field of an object literal written in
place is a compile error (`E3012`); otherwise it is a runtime error.

## Statements

```meadows
let name = expression;
if (condition) { ... } else if (condition) { ... } else { ... }
while (condition) { ... }
for (i in range(start, end)) { ... }
break;
continue;
return expression;
{ ... }
```

`for` runs `i` from `start` up to but excluding `end`; both bounds are evaluated
once and must be integers, and assigning to `i` inside the body does not affect
iteration. Braces are required. Every simple statement ends with `;`.

## Functions

```meadows
func add(a, b) {
  return a + b;
}
```

Functions are declared at the top level of a file and may be called before their
declaration. Parameters and results can be any value. A function without
`return`, or with a bare `return;`, returns `0`.

A function name used as a value is a function reference, and any expression can
be called:

```meadows
func apply(f, x) { return f(x); }
let g = add;
print(apply(g, 1));
print(table[0](2));
```

Calls to a named function are checked at compile time (`E3006`). Calls through
values are checked at run time. Calls may nest at most 10,000 deep.

## Built-in functions

| Call           | Result                                                          |
|----------------|-----------------------------------------------------------------|
| `print(v)`     | Writes an integer, float or string and a newline. Returns `0`.  |
| `len(v)`       | Length in bytes of a string, or elements of an array.           |
| `str(n)`       | Formats an integer or float as a string.                        |
| `push(a, v)`   | Appends `v` to array `a` in place and returns `a`.              |

Built-ins cannot be used as values, shadowed or redefined.

## Modules

```meadows
import "geometry.ms";
```

`import` appears at the top level and is replaced by the contents of the named
file. The path is relative to the importing file, must end in `.ms`, and must be
relative and free of `..`. All files share one global namespace, so a name
defined twice is the same error as a redefinition within one file. A file is
included only the first time it is imported, which also makes circular imports
terminate.

## Diagnostics

Compile-time diagnostics are printed with the offending source line:

```
error[E3001]: Undefined variable 'x'
 --> main.ms:3:7
  |
3 | print(x);
  |       ^
```

| Code  | Meaning                                              |
|-------|------------------------------------------------------|
| E1001 | Unterminated string literal                           |
| E1002 | Unexpected character                                  |
| E1003 | Invalid escape sequence                               |
| E1004 | Number literal out of range                           |
| E1005 | String literal too long                               |
| E2001–E2017 | Syntax errors: a specific token or construct was expected |
| E2010 | Invalid assignment target                             |
| E2014 | Nesting too deep (limit 256)                          |
| E2015 | `func` or `import` not at the top level               |
| E3001 | Undefined variable                                    |
| E3002 | Undefined function                                    |
| E3003 | Variable already declared in this scope               |
| E3004 | Function already defined                              |
| E3006 | Wrong number of arguments                             |
| E3009 | `break` outside a loop                                |
| E3010 | `continue` outside a loop                             |
| E3011 | `return` outside a function                           |
| E3012 | Unknown field on an object literal                    |
| E3013 | Name already used by a function or built-in          |
| E3014 | Duplicate field in an object literal                  |
| E3015 | Assignment to a function                              |
| E3016 | Built-in used as a value                              |
| E5001–E5004 | Import problems: missing, too large, invalid path, unreadable |
| W6001 | Variable declared but never used (silence with a `_` prefix) |
| W6003 | Unreachable statement                                 |
| W6004 | Variable shadows an outer declaration                 |

Warnings can be silenced with `-Wno-unused-variable`, `-Wno-unreachable-code` and
`-Wno-shadowing`, or promoted to errors with `-Werror`.

## Runtime errors

A runtime error stops the program, flushes standard output, prints a message to
standard error and exits with status `1`:

```
RuntimeError: Division by zero
  at main.ms:3:9
```

| Message                                                   | Cause                                  |
|-----------------------------------------------------------|----------------------------------------|
| `Division by zero`                                        | `/` or `%` by integer or float zero    |
| `Array index N out of bounds for length L`                | Index outside the array                |
| `Array index must be an integer, got T`                   | Non-integer index                      |
| `Cannot index <type>`                                     | Indexing a non-array                   |
| `Cannot access field 'f' on <type>`                       | Field access on a non-object           |
| `Object has no field 'f'`                                 | Unknown field                          |
| `Operator 'op' cannot be applied to T1 and T2`            | Operand types the operator rejects     |
| `Condition must be an integer, got T`                     | `if` or `while` condition              |
| `Range bounds must be integers, got T`                    | `for` bounds                           |
| `Cannot call <type>`                                      | Calling a non-function                 |
| `Function 'f' takes N arguments, got M`                   | Wrong argument count through a value   |
| `Maximum call depth exceeded`                             | More than 10,000 nested calls          |
| `String exceeds the maximum length`                       | Concatenation beyond 1 MiB             |
| `Array exceeds the maximum length`                        | `push` beyond 4,194,304 elements       |
| `print() cannot print <type>`                             | Printing an array, object or function  |
| `len() requires a string or an array, got T`              | Bad `len` argument                     |
| `str() requires a number, got T`                          | Bad `str` argument                     |
| `push() requires an array as its first argument, got T`   | Bad `push` argument                    |

`T` is `integer`, `float`, `string`, `array`, `object` or `function`.

# Meadows Example Programs

This directory contains example Meadows programs that demonstrate the language features.

## Running Examples

```bash
# Compile an example (see the root README for how to build the compiler)
./build-release/bin/Meadows examples/hello.ms

# Run the compiled program (the executable's name is the source file's
# stem, written to the current directory — not examples/hello.out)
./hello.out
```

## Available Examples

### hello.ms
Basic "Hello, world" program demonstrating print statements.

### variables.ms
Demonstrates variable declarations and arithmetic operations.

### factorial.ms
Recursive function example with conditionals.

### loop.ms
For loop demonstration using range.

## Adding New Examples

1. Create a new `.ms` file with your example code
2. Create a corresponding `.expected` file with the expected output
3. The example will be automatically tested by `./test.sh integration`

## Example Template

```meadows
# Your example program here
print("Hello, Meadows!");
```

Expected output file (`.expected`):
```
Hello, Meadows!
```

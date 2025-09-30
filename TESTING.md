# Testing Framework

The Meadows compiler uses a simple test-driven development approach with automated testing.

## Test Structure

Tests are located in the `tests/` directory:

- `*.ms`: Meadows source files to be compiled and executed
- `*.expected`: Expected output files for each test case

## Running Tests

Execute the test suite:

```bash
./test.sh
```

This script will:
1. Build the compiler using `./build.sh`
2. Discover all `.ms` files in `tests/` that have corresponding `.expected` files
3. Compile each `.ms` file to an executable
4. Run the executable and capture output
5. Compare output to expected results
6. Report pass/fail status for each test
7. Clean up generated `.ll` and `.out` files
8. Display overall pass percentage

## Adding New Tests

1. Create a new `.ms` file in `tests/` with your test code
2. Create a corresponding `.expected` file with the expected output
3. Run `./test.sh` to verify the test passes

## Example Test Case

`tests/hello.ms`:
```
print "Hello, world";
```

`tests/hello.ms.expected`:
```
Hello, world
```

## Test Output

```
Testing hello.ms: PASS
Testing variables.ms: PASS
...
Total: 4/4 tests passed (100%)
```

If a test fails, it shows expected vs actual output for debugging. If compilation fails, it reports "COMPILATION FAILED".
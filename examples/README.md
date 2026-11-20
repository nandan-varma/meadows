# Examples

Small programs that show the language. Each one has a `.expected` file with its
output, and the test suite runs them through both the interpreter and the
native compiler.

```bash
Meadows examples/fibonacci.ms -o fibonacci && ./fibonacci   # compile and run
Meadows --run examples/fibonacci.ms                         # interpret
Meadows --run examples/modules/main.ms                      # imports
```

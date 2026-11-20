export const BUILTIN_DOCS = {
  print: {
    signature: "print(value)",
    summary: "Writes an integer, float or string followed by a newline. Returns 0.",
    example: 'print("Hello, " + name);',
  },
  len: {
    signature: "len(value)",
    summary: "Length in bytes of a string, or number of elements of an array.",
    example: "print(len([1, 2, 3]));",
  },
  str: {
    signature: "str(number)",
    summary: "Formats an integer or float as a string.",
    example: 'print("n = " + str(n));',
  },
  push: {
    signature: "push(array, value)",
    summary: "Appends value to the array in place and returns the same array.",
    example: "push(items, 42);",
  },
};

export const KEYWORD_DOCS = {
  let: { summary: "Declares a variable in the current scope.", example: "let x = 1;" },
  func: {
    summary: "Declares a top-level function. Functions only see their parameters, locals and other functions.",
    example: "func add(a, b) {\n  return a + b;\n}",
  },
  if: { summary: "Runs a block when the integer condition is non-zero.", example: "if (x > 0) {\n  print(x);\n}" },
  else: { summary: "The alternative branch of an if; chain with else if.", example: "} else {\n  print(0);\n}" },
  for: {
    summary: "Counts i from start up to but excluding end.",
    example: "for (i in range(0, 10)) {\n  print(i);\n}",
  },
  while: { summary: "Repeats a block while the integer condition is non-zero.", example: "while (n > 0) {\n  n = n - 1;\n}" },
  return: { summary: "Leaves the function with a value; a bare return gives 0.", example: "return x;" },
  in: { summary: "Separates the loop variable from its range in a for loop.", example: "for (i in range(0, n))" },
  range: { summary: "Gives the half-open integer bounds of a for loop.", example: "range(0, 10)" },
  break: { summary: "Leaves the innermost loop.", example: "break;" },
  continue: { summary: "Skips to the next iteration of the innermost loop.", example: "continue;" },
  import: { summary: "Includes another file once, relative to this one.", example: 'import "lib.ms";' },
  true: { summary: "The integer 1.", example: "true" },
  false: { summary: "The integer 0.", example: "false" },
};

export const OPERATORS = [
  ["= ", "Assignment, right-associative"],
  ["||  &&", "Short-circuit or, and"],
  ["==  !=", "Equality, any operands"],
  ["<  <=  >  >=", "Ordering, numbers only"],
  ["+  -", "Add, subtract, concatenate strings"],
  ["*  /  %", "Multiply, divide, remainder"],
  ["-  !", "Unary negate, not"],
  ["()  []  .", "Call, index, field"],
];

export const LIMITS = [
  ["integer", "32-bit signed, wraps on overflow"],
  ["float", "64-bit IEEE 754"],
  ["string", "byte string, at most 1 MiB"],
  ["array", "at most 4,194,304 elements"],
  ["call depth", "10,000 nested calls"],
  ["nesting", "256 levels of syntax"],
];

export const SNIPPETS = [
  { label: "func", detail: "function", template: "func ${name}(${params}) {\n  ${}\n}" },
  { label: "for", detail: "range loop", template: "for (${i} in range(${0}, ${n})) {\n  ${}\n}" },
  { label: "while", detail: "loop", template: "while (${condition}) {\n  ${}\n}" },
  { label: "if", detail: "conditional", template: "if (${condition}) {\n  ${}\n}" },
  { label: "ifelse", detail: "conditional with else", template: "if (${condition}) {\n  ${}\n} else {\n  ${}\n}" },
  { label: "import", detail: "import a file", template: 'import "${file}.ms";' },
];

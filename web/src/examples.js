const file = (name, text) => ({ name, text });

export const EXAMPLES = [
  {
    id: "hello",
    title: "Hello, world",
    files: [file("main.ms", `print("Hello, Meadows!");\n`)],
  },
  {
    id: "fibonacci",
    title: "Fibonacci",
    files: [
      file(
        "main.ms",
        `func fib(n) {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

for (i in range(0, 15)) {
  print("fib(" + i + ") = " + fib(i));
}
`,
      ),
    ],
  },
  {
    id: "fizzbuzz",
    title: "FizzBuzz",
    files: [
      file(
        "main.ms",
        `for (n in range(1, 21)) {
  if (n % 15 == 0) {
    print("FizzBuzz");
  } else if (n % 3 == 0) {
    print("Fizz");
  } else if (n % 5 == 0) {
    print("Buzz");
  } else {
    print(n);
  }
}
`,
      ),
    ],
  },
  {
    id: "collections",
    title: "Arrays and objects",
    files: [
      file(
        "main.ms",
        `let people = [
  {name: "Ada", born: 1815},
  {name: "Grace", born: 1906},
  {name: "Alan", born: 1912}
];

let names = [];
for (i in range(0, len(people))) {
  names = push(names, people[i].name);
}
print("people: " + len(names));
print("first: " + names[0]);

people[0].born = 1816;
print(people[0].name + " was born in " + people[0].born);
`,
      ),
    ],
  },
  {
    id: "functions",
    title: "Functions as values",
    files: [
      file(
        "main.ms",
        `func twice(f, x) {
  return f(f(x));
}

func increment(n) {
  return n + 1;
}

func double(n) {
  return n * 2;
}

print(twice(increment, 5));
print(twice(double, 5));

let operations = {add: increment, scale: double};
print(operations.scale(21));
`,
      ),
    ],
  },
  {
    id: "linked-list",
    title: "Linked list",
    files: [
      file(
        "main.ms",
        `let list = 0;
for (i in range(1, 6)) {
  list = {value: i * i, next: list};
}

let node = list;
let total = 0;
while (node != 0) {
  print(node.value);
  total = total + node.value;
  node = node.next;
}
print("total: " + total);
`,
      ),
    ],
  },
  {
    id: "modules",
    title: "Modules",
    files: [
      file(
        "main.ms",
        `import "geometry.ms";

print("area: " + area(3, 4));
print("perimeter: " + perimeter(3, 4));
`,
      ),
      file(
        "geometry.ms",
        `func area(width, height) {
  return width * height;
}

func perimeter(width, height) {
  return 2 * (width + height);
}
`,
      ),
    ],
  },
  {
    id: "runtime-error",
    title: "Runtime error",
    files: [
      file(
        "main.ms",
        `func average(total, count) {
  return total / count;
}

print(average(10, 4));
print(average(10, 0));
print("unreachable");
`,
      ),
    ],
  },
];

export function findExample(id) {
  return EXAMPLES.find((example) => example.id === id);
}

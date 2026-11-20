func twice(f, x) {
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

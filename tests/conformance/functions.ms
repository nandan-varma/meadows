func add(a, b) {
  return a + b;
}
func greet(name) {
  return "Hello, " + name + "!";
}
func noReturn() {
}
func bareReturn() {
  return;
}
func isEven(n) {
  if (n == 0) { return 1; }
  return isOdd(n - 1);
}
func isOdd(n) {
  if (n == 0) { return 0; }
  return isEven(n - 1);
}
func fact(n) {
  if (n <= 1) { return 1; }
  return n * fact(n - 1);
}
func fib(n) {
  if (n < 2) { return n; }
  return fib(n - 1) + fib(n - 2);
}
func first(items) {
  return items[0];
}
func fill(items, v) {
  for (i in range(0, len(items))) {
    items[i] = v;
  }
}
func mean(a, b) {
  return (a + b) / 2.0;
}
func pair(a, b) {
  return {first: a, second: b};
}
print(add(2, 3));
print(add("a", "b"));
print(add(1.5, 2));
print(greet("Meadows"));
print(noReturn());
print(bareReturn());
print(isEven(10));
print(isOdd(7));
print(fact(10));
print(fib(20));
print(first(["x", "y"]));
let xs = [1, 2, 3];
fill(xs, 7);
print(xs[2]);
print(mean(3, 4));
print(pair("a", 2).second);
print(later(5));
func later(n) {
  return n * 2;
}

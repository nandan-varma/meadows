func add(a, b) { return a + b; }
func mul(a, b) { return a * b; }
func apply(f, x, y) { return f(x, y); }
func pick(useAdd) {
  if (useAdd) { return add; }
  return mul;
}
func counterStep() { return counterStep; }
let f = add;
print(f(3, 4));
f = mul;
print(f(3, 4));
print(apply(add, 5, 6));
print(apply(mul, 5, 6));
print(pick(1)(2, 3));
print(pick(0)(2, 3));
let table = [add, mul];
print(table[0](10, 20));
print(table[1](10, 20));
let ops = {plus: add, times: mul};
print(ops.plus(1, 2));
print(ops.times(3, 4));
print(add == add);
print(add == mul);
print(f == mul);
print(counterStep()()() == counterStep);

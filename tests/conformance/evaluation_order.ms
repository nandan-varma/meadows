func t(n) {
  print(n);
  return n;
}
let o = {b: t(1), a: t(2), c: t(3)};
let a = [t(4), t(5)];
print(t(6) + t(7) * t(8));
print(t(0) && t(9));
print(t(10) || t(11));
print(t(0) || t(12));
print(t(13) && t(14));
let arr = [0, 0];
arr[t(0)] = t(15);
print(arr[0]);
let f = t;
f(t(16));

let total = 0;
for (i in range(0, 300000)) {
  total = total + i % 7;
}
print(total);
let s = "";
for (i in range(0, 2000)) {
  s = s + "x";
}
print(len(s));
let a = [];
for (i in range(0, 50000)) {
  a = push(a, i);
}
print(len(a));
print(a[49999]);

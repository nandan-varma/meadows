func classify(n) {
  if (n < 0) {
    return "negative";
  } else if (n == 0) {
    return "zero";
  } else if (n < 10) {
    return "small";
  } else {
    return "large";
  }
}
print(classify(-5));
print(classify(0));
print(classify(7));
print(classify(70));

let i = 0;
while (i < 3) {
  print(i);
  i = i + 1;
}

for (j in range(0, 5)) {
  if (j == 1) { continue; }
  if (j == 4) { break; }
  print(j);
}

for (a in range(0, 3)) {
  for (b in range(0, 3)) {
    if (b == 2) { break; }
    if (a == 1) { continue; }
    print(a * 10 + b);
  }
}

for (k in range(3, 3)) { print(k); }
for (k in range(5, 3)) { print(k); }
for (k in range(-2, 1)) { print(k); }
for (k in range(2147483646, 2147483647)) { print(k); }

func findFirstOver(limit) {
  for (n in range(0, 100)) {
    while (1) {
      if (n * n > limit) { return n; }
      break;
    }
  }
  return -1;
}
print(findFirstOver(50));
print(findFirstOver(100000));

let total = 0;
let n = 0;
while (n < 1000) {
  n = n + 1;
  if (n % 2 == 0) { continue; }
  total = total + n;
}
print(total);

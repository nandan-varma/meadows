func sum(n) {
  if (n == 0) { return 0; }
  return n + sum(n - 1);
}
print(sum(9000));
func count(n, acc) {
  if (n == 0) { return acc; }
  return count(n - 1, acc + 1);
}
print(count(9999, 0));

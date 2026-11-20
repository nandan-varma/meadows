func forever(n) {
  return forever(n + 1) + 1;
}
print(forever(0));

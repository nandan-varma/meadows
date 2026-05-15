func add(a, b) {
    return a + b;
}

func multiply(a, b) {
    return a * b;
}

func recursive(n) {
    if (n <= 1) {
        return n;
    }
    return n + recursive(n - 1);
}

print(add(5, 3));
print(multiply(4, 7));
print(recursive(10));

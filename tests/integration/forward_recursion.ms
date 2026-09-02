func isEven(n) {
    if (n == 0) {
        return 1;
    }
    return isOdd(n - 1);
}

func isOdd(n) {
    if (n == 0) {
        return 0;
    }
    return isEven(n - 1);
}

print(isEven(10));
print(isOdd(10));

func callsLater() {
    return definedAfter();
}

func definedAfter() {
    return 99;
}

print(callsLater());

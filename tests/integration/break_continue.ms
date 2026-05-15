let count = 0;
for (i in range(0, 10)) {
    if (i == 5) {
        break;
    }
    count = count + 1;
}
print(count);

let sum = 0;
for (j in range(0, 10)) {
    if (j == 5) {
        continue;
    }
    sum = sum + j;
}
print(sum);

let list = 0;
for (i in range(1, 6)) {
  list = {value: i * i, next: list};
}

let node = list;
let total = 0;
while (node != 0) {
  print(node.value);
  total = total + node.value;
  node = node.next;
}
print("total: " + total);

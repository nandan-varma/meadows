let list = 0;
for (i in range(0, 200000)) {
  list = {next: list, value: i};
}
print(list.value);
let nest = [];
for (i in range(0, 200000)) {
  nest = [nest];
}
print(len(nest));
print("released");

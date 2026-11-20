func rhs() {
  print("rhs evaluated");
  return 1;
}
let a = [0];
a[3] = rhs();

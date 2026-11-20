let x = 1;
{
  let y = 2;
  print(x + y);
  {
    let x = 10;
    print(x + y);
  }
  print(x);
}
let y = 100;
print(y);
for (i in range(0, 2)) {
  let local = i * 2;
  print(local);
}
let i = 50;
print(i);
if (1) {
  let inner = "in";
  print(inner);
}
let inner = "out";
print(inner);
func shadowParam(x) {
  { let x = x + 1; return x; }
}
print(shadowParam(5));
print(x);

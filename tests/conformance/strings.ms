let a = "foo";
let b = "bar";
print(a + b);
print(a + " " + b);
print(len(a + b));
print("n=" + 5);
print(5 + "=n");
print("pi=" + 3.5);
print("neg=" + -12);
print(a == "foo");
print(a != b);
print(a == b);
print("" == "");
print(len(""));
print("tab\there");
print("quote\"inside");
print("back\\slash");
print("multi\nline");
print(len("héllo"));
print(str(42) + str(-7));
print(str(2.5));
print(len(str(1000000)));
let s = "";
for (i in range(0, 5)) {
  s = s + str(i) + ",";
}
print(s);

let a = [1];
a[0] = a;
print(len(a));
print(a[0] == a);
let o = {self: 0};
o.self = o;
print(o.self.self.self == o);
print("done");

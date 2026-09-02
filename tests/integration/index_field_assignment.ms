let arr = [1, 2, 3, 4, 5];
arr[0] = 100;
arr[4] = 500;
print(arr[0]);
print(arr[1]);
print(arr[4]);

let o = {a: 1, b: 2};
o.a = 100;
print(o.a);
print(o.b);

let alias = o;
alias.b = 200;
print(o.b);
print(alias.b);

let people = [
  {name: "Ada", born: 1815},
  {name: "Grace", born: 1906},
  {name: "Alan", born: 1912}
];

let names = [];
for (i in range(0, len(people))) {
  names = push(names, people[i].name);
}
print("people: " + len(names));
print("first: " + names[0]);

people[0].born = 1816;
print(people[0].name + " was born in " + people[0].born);

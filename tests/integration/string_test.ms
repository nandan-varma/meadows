import std.string;

func main() {
    let s1 = "Hello";
    let len = std.string.strlen(s1);
    print(len);

    let s2 = "World";
    let result = std.string.strcmp(s1, s2);
    print(result);

    return 0;
}

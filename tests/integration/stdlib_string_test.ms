import std.string;

func main() {
    print("=== String Stdlib Tests ===");

    let s1 = "Hello";

    let len1 = std.string.strlen(s1);
    print(len1);

    let cmp = std.string.strcmp("Hello", "World");
    print(cmp);

    let dup = std.string.strdup("Test");
    let dup_len = std.string.strlen(dup);
    print(dup_len);

    print("=== String Tests Complete ===");
    return 0;
}

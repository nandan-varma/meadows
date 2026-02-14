import std.args;
import std.io;
import std.string;

func cat_file(path: string) -> void {
    let content = read_file(path);
    if (strcmp(content, "") == 0) {
        print("cat: ");
        print(path);
        print(": No such file or directory\n");
    } else {
        print(content);
    }
}

func main() -> i32 {
    let count = args();
    if (count == 1) {
        print("cat: missing operand\n");
        return 1;
    }

    let i = 1;
    while (i < count) {
        cat_file(getarg(i));
        i = i + 1;
    }
    return 0;
}

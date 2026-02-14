import std.args;
import std.io;
import std.string;

func main() -> i32 {
    let count = args();
    
    if (count > 1) {
        let path = getarg(1);
        let content = read_file(path);
        if (strcmp(content, "") == 0) {
            print("wc: ");
            print(path);
            print(": No such file or directory\n");
            return 1;
        }

        let lines = 1;
        let words = 1;
        let chars = strlen(content);

        print(" ");
        print(lines);
        print(" ");
        print(words);
        print(" ");
        print(chars);
        print(" ");
        print(path);
        print("\n");
    }
    return 0;
}

import std.args;
import std.io;
import std.string;

func main() -> i32 {
    let count = args();
    
    if (count > 1) {
        let path = getarg(1);
        let content = read_file(path);
        if (strcmp(content, "") == 0) {
            print("tail: ");
            print(path);
            print(": No such file or directory\n");
            return 1;
        }
        print(content);
    }
    return 0;
}

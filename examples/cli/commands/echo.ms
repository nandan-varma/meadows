import std.args;
import std.string;

func main() -> i32 {
    let count = args();
    if (count <= 1) {
        print("");
        return 0;
    }
    
    let result = getarg(1);
    let i = 2;
    while (i < count) {
        result = result + " " + getarg(i);
        i = i + 1;
    }
    print(result);
    return 0;
}

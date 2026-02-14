import std.args;
import std.dir;
import std.io;
import std.string;

func main() -> i32 {
    let count = args();
    let path = ".";
    
    if (count > 1) {
        path = getarg(1);
    }
    
    let dir_ptr = opendir(path);
    
    if (dir_ptr == 0) {
        print("ls: cannot open directory\n");
        return 1;
    }
    
    while (1) {
        let entry = readdir(dir_ptr);
        if (strcmp(entry, "") == 0) {
            break;
        }
        print(entry);
        print("\n");
    }
    
    closedir(dir_ptr);
    return 0;
}

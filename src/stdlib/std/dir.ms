module std.dir;

extern "meadows_opendir" opendir(path: string) -> i64;
extern "meadows_readdir" readdir(dir_ptr: i64) -> string;
extern "meadows_closedir" closedir(dir_ptr: i64) -> i32;
extern "meadows_mkdir" mkdir(path: string, mode: i32) -> i32;
extern "meadows_rmdir" rmdir(path: string) -> i32;
extern "meadows_is_directory" is_directory(path: string) -> i32;

func is_dir(path: string) -> i32 {
    return is_directory(path);
}

func is_file(path: string) -> i32 {
    let exists = file_exists(path);
    if (exists == 0) {
        return 0;
    }
    return is_directory(path) == 0;
}

func exists(path: string) -> i32 {
    return file_exists(path);
}

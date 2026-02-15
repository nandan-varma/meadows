module std.panic;

extern "meadows_print_string" __print_string(msg: string) -> i32;
extern "meadows_exit" __exit(code: i32) -> void;

func panic(message: string) -> i32 {
    __print_string("PANIC: ");
    __print_string(message);
    __print_string("\n");
    __exit(1);
    return 1;
}

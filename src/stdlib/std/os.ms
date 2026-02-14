module std.os;

extern "meadows_getenv" getenv(name: string) -> string;
extern "meadows_setenv" setenv(name: string, value: string) -> i32;
extern "meadows_exit" exit(code: i32) -> void;

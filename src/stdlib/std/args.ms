module std.args;

extern "meadows_args" args() -> i32;
extern "meadows_getarg" getarg(n: i32) -> string;

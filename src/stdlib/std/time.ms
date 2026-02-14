module std.time;

extern "meadows_time" time() -> i64;
extern "meadows_sleep" sleep(milliseconds: i64) -> void;
extern "meadows_clock" clock() -> i64;

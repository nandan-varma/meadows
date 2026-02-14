module std.array;

extern "meadows_array_length" array_length(arr: i64) -> i32;
extern "meadows_array_create" array_create(size: i32) -> i64;
extern "meadows_array_copy" array_copy(src: i64, dest: i64, size: i32) -> i64;
extern "meadows_array_fill" array_fill(arr: i64, value: i32, size: i32) -> void;
extern "meadows_array_index_of" array_index_of(arr: i64, value: i32, size: i32) -> i32;
extern "meadows_array_contains" array_contains(arr: i64, value: i32, size: i32) -> i32;
extern "meadows_array_sort" array_sort(arr: i64, size: i32) -> void;

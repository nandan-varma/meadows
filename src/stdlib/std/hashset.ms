extern "meadows_hashset_create" hashset_create(capacity: i32) -> i64;
extern "meadows_hashset_free" hashset_free(set: i64) -> void;
extern "meadows_hashset_len" hashset_len(set: i64) -> i32;
extern "meadows_hashset_add" hashset_add(set: i64, value: i32) -> i64;
extern "meadows_hashset_has" hashset_has(set: i64, value: i32) -> i32;
extern "meadows_hashset_remove" hashset_remove(set: i64, value: i32) -> i64;

func hashset_new() -> i64 {
    return hashset_create(0);
}

func hashset_with_capacity(capacity: i32) -> i64 {
    return hashset_create(capacity);
}

func hashset_len(s: i64) -> i32 {
    return hashset_len(s);
}

func hashset_add(s: i64, value: i32) -> i64 {
    return hashset_add(s, value);
}

func hashset_has(s: i64, value: i32) -> i32 {
    return hashset_has(s, value);
}

func hashset_remove(s: i64, value: i32) -> i64 {
    return hashset_remove(s, value);
}

func hashset_free(s: i64) -> void {
    hashset_free(s);
}

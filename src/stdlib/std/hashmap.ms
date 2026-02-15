extern "meadows_hashmap_create" hashmap_create(capacity: i32) -> i64;
extern "meadows_hashmap_free" hashmap_free(map: i64) -> void;
extern "meadows_hashmap_len" hashmap_len(map: i64) -> i32;
extern "meadows_hashmap_put" hashmap_put(map: i64, key: string, value: i32) -> i64;
extern "meadows_hashmap_get" hashmap_get(map: i64, key: string) -> i32;
extern "meadows_hashmap_has" hashmap_has(map: i64, key: string) -> i32;
extern "meadows_hashmap_remove" hashmap_remove(map: i64, key: string) -> i64;

func hashmap_new() -> i64 {
    return hashmap_create(0);
}

func hashmap_with_capacity(capacity: i32) -> i64 {
    return hashmap_create(capacity);
}

func hashmap_len(m: i64) -> i32 {
    return hashmap_len(m);
}

func hashmap_put(m: i64, key: string, value: i32) -> i64 {
    return hashmap_put(m, key, value);
}

func hashmap_get(m: i64, key: string) -> i32 {
    return hashmap_get(m, key);
}

func hashmap_has(m: i64, key: string) -> i32 {
    return hashmap_has(m, key);
}

func hashmap_remove(m: i64, key: string) -> i64 {
    return hashmap_remove(m, key);
}

func hashmap_free(m: i64) -> void {
    hashmap_free(m);
}

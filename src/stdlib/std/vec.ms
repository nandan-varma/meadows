extern "meadows_vec_create" vec_create(capacity: i32) -> i64;
extern "meadows_vec_free" vec_free(vec: i64) -> void;
extern "meadows_vec_len" vec_len(vec: i64) -> i32;
extern "meadows_vec_push" vec_push(vec: i64, value: i32) -> i64;
extern "meadows_vec_get" vec_get(vec: i64, index: i32) -> i32;
extern "meadows_vec_set" vec_set(vec: i64, index: i32, value: i32) -> void;

func vec_new() -> i64 {
    return vec_create(0);
}

func vec_with_capacity(capacity: i32) -> i64 {
    return vec_create(capacity);
}

func vec_len(v: i64) -> i32 {
    return vec_len(v);
}

func vec_push(v: i64, value: i32) -> i64 {
    return vec_push(v, value);
}

func vec_get(v: i64, index: i32) -> i32 {
    return vec_get(v, index);
}

func vec_set(v: i64, index: i32, value: i32) -> void {
    vec_set(v, index, value);
}

func vec_free(v: i64) -> void {
    vec_free(v);
}

module std.string;

extern "meadows_strlen" strlen(s: string) -> i64;
extern "meadows_strcpy" strcpy(dest: string, src: string) -> string;
extern "meadows_strcat" strcat(dest: string, src: string) -> string;
extern "meadows_strcmp" strcmp(a: string, b: string) -> i32;
extern "meadows_strncmp" strncmp(a: string, b: string, n: i64) -> i64;
extern "meadows_strdup" strdup(s: string) -> string;
extern "meadows_strnlen" strnlen(s: string, n: i64) -> i64;
extern "meadows_memcpy" memcpy(dest: string, src: string, n: i64) -> string;
extern "meadows_memcmp" memcmp(a: string, b: string, n: i64) -> i64;
extern "meadows_alloc_string" alloc_string(size: i64) -> string;
extern "meadows_transform_case" transform_case(s: string, mode: i32) -> void;
extern "meadows_append_string" append_string(dest: string, src: string) -> void;

extern "meadows_index_of" index_of(haystack: string, needle: string) -> i32;
extern "meadows_last_index_of" last_index_of(haystack: string, needle: string) -> i32;
extern "meadows_contains" contains(haystack: string, needle: string) -> i32;
extern "meadows_starts_with" starts_with(s: string, prefix: string) -> i32;
extern "meadows_ends_with" ends_with(s: string, suffix: string) -> i32;
extern "meadows_substring" substring(s: string, start: i32, length: i32) -> string;
extern "meadows_trim" trim(s: string) -> string;
extern "meadows_trim_left" trim_left(s: string) -> string;
extern "meadows_trim_right" trim_right(s: string) -> string;
extern "meadows_reverse" reverse(s: string) -> string;
extern "meadows_replace" replace(s: string, old_str: string, to: string) -> string;
extern "meadows_to_upper" to_upper(s: string) -> string;
extern "meadows_to_lower" to_lower(s: string) -> string;
extern "meadows_format_int" format_int(n: i32) -> string;
extern "meadows_format_float" format_float(n: f64, decimals: i32) -> string;

func length(s: string) -> i32 {
    let result = strlen(s);
    return result;
}

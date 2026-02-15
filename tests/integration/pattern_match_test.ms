func test_match(x: i32) -> i32 {
    let result = match (x) {
        0 => 42,
        _ => 99,
    };
    return result;
}

func main() -> i32 {
    let r0 = test_match(0);
    let r5 = test_match(5);
    print r0;
    print r5;
    return r0;
}

func test(x: i32) -> i32 {
    return match (x) {
        1 | 2 => 10,
        _ => 0,
    };
}

func main() -> i32 {
    let r1 = test(1);
    let r2 = test(2);
    let r3 = test(3);
    
    print r1;
    print r2;
    print r3;
    
    return r1;
}

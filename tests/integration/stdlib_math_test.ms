import std.math;

func main() {
    print("Testing math functions...");

    let abs_neg = std.math.iabs(-42);
    print(abs_neg);

    let min_val = std.math.min_i32(10, 20);
    print(min_val);

    let max_val = std.math.max_i32(10, 20);
    print(max_val);

    std.math.srand(12345);
    let rand1 = std.math.rand();
    print(rand1);

    return 0;
}

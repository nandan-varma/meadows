# Benchmark: matrix_mult_bench.ms
# Purpose: Measure numeric computation performance
# Target: 100x100 matrix multiplication < 1 second

func create_matrix(size, init_val) {
    let mat = [];
    for (i in range(0, size)) {
        let row = [];
        for (j in range(0, size)) {
            row = row + [init_val + i + j];
        }
        mat = mat + [row];
    }
    return mat;
}

func mat_mult(a, b, size) {
    let result = [];
    for (i in range(0, size)) {
        let row = [];
        for (j in range(0, size)) {
            let sum = 0;
            for (k in range(0, size)) {
                sum = sum + a[i][k] * b[k][j];
            }
            row = row + [sum];
        }
        result = result + [row];
    }
    return result;
}

func main() {
    let size = 50;  # 50x50 matrices
    let a = create_matrix(size, 1);
    let b = create_matrix(size, 2);
    
    let c = mat_mult(a, b, size);
    
    # Verify computation worked
    print c[0][0];
    return 0;
}

struct ResultI32 {
    _tag: i32,
    value: i32,
    error: i32,
}

func __result_ok_i32(value: i32) -> ResultI32 {
    let res: ResultI32 = {_tag: 0, value: value, error: 0};
    return res;
}

func __result_err_i32(error: i32) -> ResultI32 {
    let res: ResultI32 = {_tag: 1, value: 0, error: error};
    return res;
}

func result_is_ok_i32(res: ResultI32) -> bool {
    return res._tag == 0;
}

func result_is_err_i32(res: ResultI32) -> bool {
    return res._tag == 1;
}

func result_unwrap_i32(res: ResultI32) -> i32 {
    if (res._tag == 1) {
        panic("Result is Err");
    }
    return res.value;
}

func result_unwrap_err_i32(res: ResultI32) -> i32 {
    if (res._tag == 0) {
        panic("Result is Ok");
    }
    return res.error;
}

func result_unwrap_or_i32(res: ResultI32, default: i32) -> i32 {
    if (res._tag == 1) {
        return default;
    }
    return res.value;
}

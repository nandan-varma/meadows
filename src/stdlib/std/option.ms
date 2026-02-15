struct OptionI32 {
    _tag: i32,
    value: i32,
}

func __option_some_i32(value: i32) -> OptionI32 {
    let opt: OptionI32 = {_tag: 0, value: value};
    return opt;
}

func __option_none_i32() -> OptionI32 {
    let opt: OptionI32 = {_tag: 1, value: 0};
    return opt;
}

func option_is_some_i32(opt: OptionI32) -> bool {
    return opt._tag == 0;
}

func option_is_none_i32(opt: OptionI32) -> bool {
    return opt._tag == 1;
}

func option_unwrap_i32(opt: OptionI32) -> i32 {
    if (opt._tag == 1) {
        panic("Option is None");
    }
    return opt.value;
}

func option_unwrap_or_i32(opt: OptionI32, default: i32) -> i32 {
    if (opt._tag == 1) {
        return default;
    }
    return opt.value;
}

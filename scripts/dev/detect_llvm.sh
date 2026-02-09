#!/bin/bash
# Auto-detect LLVM 17 installation
# Usage: source scripts/dev/detect_llvm.sh && get_llvm_dir

get_llvm_dir() {
    local llvm_dir=""

    if [[ -n "$LLVM_DIR" ]]; then
        llvm_dir="$LLVM_DIR"
    elif [[ -d "${GITHUB_WORKSPACE}/llvm/lib/cmake/llvm" ]]; then
        # KyleMayes action installs here
        llvm_dir="${GITHUB_WORKSPACE}/llvm/lib/cmake/llvm"
    elif [[ "$(uname)" == "Darwin" ]]; then
        if command -v brew >/dev/null 2>&1; then
            if brew list llvm@17 >/dev/null 2>&1; then
                llvm_dir="$(brew --prefix llvm@17)/lib/cmake/llvm"
            elif brew list llvm >/dev/null 2>&1; then
                llvm_dir="$(brew --prefix llvm)/lib/cmake/llvm"
            fi
        fi
    elif [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == CYGWIN* ]]; then
        if [[ -f "/c/Program Files/LLVM/lib/cmake/llvm/LLVMConfig.cmake" ]]; then
            llvm_dir="/c/Program Files/LLVM/lib/cmake/llvm"
        elif [[ -f "/c/Program Files (x86)/LLVM/lib/cmake/llvm/LLVMConfig.cmake" ]]; then
            llvm_dir="/c/Program Files (x86)/LLVM/lib/cmake/llvm"
        fi
    elif [[ -f "/usr/lib/llvm-17/lib/cmake/llvm/LLVMConfig.cmake" ]]; then
        llvm_dir="/usr/lib/llvm-17/lib/cmake/llvm"
    elif [[ -f "/usr/lib/llvm-16/lib/cmake/llvm/LLVMConfig.cmake" ]]; then
        llvm_dir="/usr/lib/llvm-16/lib/cmake/llvm"
    elif [[ -f "/usr/lib/llvm-15/lib/cmake/llvm/LLVMConfig.cmake" ]]; then
        llvm_dir="/usr/lib/llvm-15/lib/cmake/llvm"
    fi

    echo "$llvm_dir"
}

get_llvm_runtime_dir() {
    local runtime_dir=""

    if [[ -d "${GITHUB_WORKSPACE}/llvm/lib" ]]; then
        # KyleMayes action installs here
        runtime_dir="${GITHUB_WORKSPACE}/llvm/lib"
    elif [[ "$(uname)" == "Darwin" ]]; then
        if command -v brew >/dev/null 2>&1; then
            if brew list llvm@17 >/dev/null 2>&1; then
                runtime_dir="$(brew --prefix llvm@17)/lib"
            elif brew list llvm >/dev/null 2>&1; then
                runtime_dir="$(brew --prefix llvm)/lib"
            fi
        fi
    elif [[ -d "/usr/lib/llvm-17/lib" ]]; then
        runtime_dir="/usr/lib/llvm-17/lib"
    elif [[ -d "/usr/lib/x86_64-linux-gnu/llvm-17/lib" ]]; then
        runtime_dir="/usr/lib/x86_64-linux-gnu/llvm-17/lib"
    fi

    echo "$runtime_dir"
}

get_cmake_jobs() {
    if [[ "$(uname)" == "Darwin" ]]; then
        sysctl -n hw.ncpu 2>/dev/null || echo 4
    else
        nproc 2>/dev/null || echo 4
    fi
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "LLVM_DIR=$(get_llvm_dir)"
    echo "LLVM_RUNTIME_DIR=$(get_llvm_runtime_dir)"
    echo "CMAKE_JOBS=$(get_cmake_jobs)"
fi

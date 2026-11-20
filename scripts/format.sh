#!/usr/bin/env bash
# Usage: scripts/format.sh [--check]
# Formats the C and C++ sources with clang-format 17 (CLANG_FORMAT overrides the binary).
set -eu

cd "$(dirname "$0")/.."
FORMATTER=${CLANG_FORMAT:-clang-format}
files=$(git ls-files 'src/*.cpp' 'src/*.h' 'src/*.c' 'tests/*.cpp' 'tests/*.h')

if [[ ${1:-} == --check ]]; then
  # shellcheck disable=SC2086
  $FORMATTER --dry-run --Werror $files
else
  # shellcheck disable=SC2086
  $FORMATTER -i $files
fi

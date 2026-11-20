#!/usr/bin/env bash
# Usage: run_conformance.sh <Meadows binary> <interpret|native|diagnostics> [--update]
set -u

BINARY=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
MODE=$2
UPDATE=${3:-}
SUITE="$(cd "$(dirname "$0")/conformance" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
QUIET=(-Wno-unused-variable -Wno-shadowing -Wno-unreachable-code)

passed=0
failed=0

report() {
  if [[ $1 == ok ]]; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL $2: $3"
  fi
}

compare() {
  local name=$1 label=$2 expected=$3 actual=$4
  if [[ $UPDATE == --update ]]; then
    if [[ -s $actual ]]; then cp "$actual" "$expected"; else rm -f "$expected"; fi
    return 0
  fi
  local reference=$expected
  [[ -f $expected ]] || reference=/dev/null
  if ! diff -u "$reference" "$actual" > "$WORK/diff"; then
    report fail "$name" "$label differs"
    head -n 20 "$WORK/diff"
    return 1
  fi
  return 0
}

execute() {
  local file=$1
  case $MODE in
    interpret)
      "$BINARY" --run "${QUIET[@]}" "$file" > "$WORK/stdout" 2> "$WORK/stderr"
      ;;
    native)
      if ! "$BINARY" "${QUIET[@]}" "$file" -o "$WORK/program" 2> "$WORK/compile"; then
        cat "$WORK/compile"
        return 125
      fi
      "$WORK/program" > "$WORK/stdout" 2> "$WORK/stderr"
      ;;
  esac
}

cd "$SUITE"

run_programs() {
  local file name code expected_code
  for file in "$@"; do
    name=${file%.ms}
    expected_code=0
    [[ -f $name.expected_error ]] && expected_code=1
    execute "$file"
    code=$?
    if [[ $code -eq 125 ]]; then
      report fail "$name" "did not compile"
      continue
    fi
    if [[ $UPDATE != --update && $code -ne $expected_code ]]; then
      report fail "$name" "exit code $code, expected $expected_code"
      continue
    fi
    compare "$name" stdout "$name.expected" "$WORK/stdout" || continue
    compare "$name" stderr "$name.expected_error" "$WORK/stderr" || continue
    report ok "$name" ""
  done
}

run_diagnostics() {
  local file name
  for file in diagnostics/*.ms; do
    name=${file%.ms}
    "$BINARY" --dump-ast "$file" > /dev/null 2> "$WORK/stderr"
    compare "$name" diagnostics "$name.expected_error" "$WORK/stderr" || continue
    report ok "$name" ""
  done
}

case $MODE in
  interpret | native)
    run_programs *.ms errors/*.ms
    ;;
  diagnostics)
    run_diagnostics
    ;;
  *)
    echo "unknown mode: $MODE" >&2
    exit 2
    ;;
esac

echo "$MODE: $passed passed, $failed failed"
[[ $failed -eq 0 ]]

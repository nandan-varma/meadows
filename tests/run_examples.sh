#!/usr/bin/env bash
# Usage: run_examples.sh <Meadows binary> <interpret|native> [--update]
set -u

BINARY=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
MODE=$2
UPDATE=${3:-}
EXAMPLES="$(cd "$(dirname "$0")/../examples" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

passed=0
failed=0
cd "$EXAMPLES"

for file in *.ms */main.ms; do
  expected=${file%.ms}.expected
  if [[ $MODE == interpret ]]; then
    "$BINARY" --run "$file" > "$WORK/stdout" 2> "$WORK/stderr"
  else
    "$BINARY" "$file" -o "$WORK/program" 2> "$WORK/stderr" && "$WORK/program" > "$WORK/stdout" 2>> "$WORK/stderr"
  fi
  status=$?
  if [[ $UPDATE == --update ]]; then
    cp "$WORK/stdout" "$expected"
  elif [[ $status -ne 0 || -s $WORK/stderr ]] || ! diff -q "$expected" "$WORK/stdout" > /dev/null; then
    failed=$((failed + 1))
    echo "FAIL $file"
    cat "$WORK/stderr"
    diff "$expected" "$WORK/stdout" | head -10
    continue
  fi
  passed=$((passed + 1))
done

echo "examples ($MODE): $passed passed, $failed failed"
[[ $failed -eq 0 ]]

#!/usr/bin/env python3
"""Runs random programs through the interpreter and the native compiler and
requires identical stdout, stderr and exit status.

Usage: differential.py <Meadows binary> [--programs N] [--seed S]
"""

import argparse
import os
import random
import subprocess
import sys
import tempfile

LEAVES = [
    "0", "1", "2", "7", "-1", "-5", "255", "65536", "2147483647", "-2147483648",
    "0.5", "1.5", "-2.25", "100.0", "0.1", "3.0",
    '"a"', '"hello"', '""', '"x\\ty"',
    "[1, 2, 3]", "[]", "{a: 1}", "{a: 2, b: \"s\"}", "f", "g",
]
BINARY = ["+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=", "&&", "||"]
UNARY = ["-", "!"]


def expression(rng, depth):
    if depth == 0 or rng.random() < 0.25:
        return rng.choice(LEAVES)
    roll = rng.random()
    if roll < 0.6:
        return "(%s %s %s)" % (expression(rng, depth - 1), rng.choice(BINARY),
                               expression(rng, depth - 1))
    if roll < 0.7:
        return "%s(%s)" % (rng.choice(UNARY), expression(rng, depth - 1))
    if roll < 0.78:
        return "len(%s)" % expression(rng, depth - 1)
    if roll < 0.86:
        return "str(%s)" % expression(rng, depth - 1)
    if roll < 0.92:
        return "[%s, %s][%s]" % (expression(rng, depth - 1),
                                 expression(rng, depth - 1),
                                 rng.choice(["0", "1", "2", "-1"]))
    return "f(%s)" % expression(rng, depth - 1)


INTS = ["0", "1", "2", "3", "7", "10", "255", "65536", "46341", "2147483647",
        "-1", "-3", "-2147483648", "(0 - 2147483647)"]
FLOATS = ["0.5", "1.5", "2.25", "100.0", "0.1", "3.0", "-0.5", "1000000.0",
          "0.000001", "123456.789"]
WORDS = ['"a"', '"hello"', '""', '"x y"', '"0"', '"tab\\t"']


def number(rng, depth, floats):
    if depth == 0 or rng.random() < 0.3:
        return rng.choice(FLOATS if floats and rng.random() < 0.6 else INTS)
    left = number(rng, depth - 1, floats)
    right = number(rng, depth - 1, floats)
    operator = rng.choice(["+", "-", "*", "/", "%"])
    if operator in "/%":
        right = rng.choice(["1", "2", "-1", "3", "7", "0.5", "-2.5"])
    return "(%s %s %s)" % (left, operator, right)


def text(rng, depth):
    if depth == 0 or rng.random() < 0.3:
        return rng.choice(WORDS)
    roll = rng.random()
    if roll < 0.4:
        return "(%s + %s)" % (text(rng, depth - 1), text(rng, depth - 1))
    if roll < 0.7:
        return "(%s + %s)" % (text(rng, depth - 1), number(rng, 1, True))
    return "str(%s)" % number(rng, depth - 1, True)


def typed_statement(rng):
    roll = rng.random()
    if roll < 0.45:
        return "print(%s);" % number(rng, rng.randint(1, 4), True)
    if roll < 0.7:
        return "print(%s);" % text(rng, rng.randint(1, 3))
    if roll < 0.85:
        return "print(%s %s %s);" % (number(rng, 2, True),
                                     rng.choice(["==", "!=", "<", "<=", ">", ">="]),
                                     number(rng, 2, True))
    return "print(len(%s));" % text(rng, 2)


def typed_program(rng):
    return "\n".join(typed_statement(rng) for _ in range(rng.randint(10, 40))) + "\n"


def program(rng):
    if rng.random() < 0.5:
        return typed_program(rng)
    lines = [
        "func f(x) { return x; }",
        "func g(x) { return x + 1; }",
    ]
    for _ in range(rng.randint(5, 25)):
        lines.append("print(%s);" % expression(rng, rng.randint(1, 4)))
    return "\n".join(lines) + "\n"


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, timeout=60, **kwargs)
    return result.returncode, result.stdout, result.stderr


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    parser.add_argument("--programs", type=int, default=100)
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()

    binary = os.path.abspath(args.binary)
    rng = random.Random(args.seed)
    quiet = ["-Wno-unused-variable", "-Wno-shadowing", "-Wno-unreachable-code"]
    mismatches = 0
    with tempfile.TemporaryDirectory() as scratch:
        os.chdir(scratch)
        for index in range(args.programs):
            name = "case%d.ms" % index
            source = program(rng)
            with open(name, "w") as handle:
                handle.write(source)
            expected = run([binary, "--run"] + quiet + [name])
            if run([binary] + quiet + [name, "-o", "case"])[0] != 0:
                print("native compilation failed for %s" % name)
                print(source)
                mismatches += 1
                continue
            actual = run(["./case"])
            if expected != actual:
                mismatches += 1
                print("MISMATCH in %s (seed %d)" % (name, args.seed))
                print(source)
                print("interpreter:", expected)
                print("native:     ", actual)
    print("%d programs, %d mismatches" % (args.programs, mismatches))
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())

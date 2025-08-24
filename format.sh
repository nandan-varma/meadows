#!/bin/bash

# Script to format all C++ source files using clang-format
# Uses the .clang-format configuration file in the project root

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed or not in PATH"
    echo "Please install clang-format first:"
    echo "  macOS: brew install clang-format"
    echo "  Ubuntu/Debian: sudo apt install clang-format"
    echo "  CentOS/RHEL: sudo yum install clang-tools-extra"
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Change to the project root directory
cd "$SCRIPT_DIR" || exit 1

# Check if .clang-format file exists
if [ ! -f ".clang-format" ]; then
    echo "Error: .clang-format file not found in project root"
    exit 1
fi

echo "Formatting C++ files using .clang-format configuration..."

# Find and format all C++ source and header files
# Include common C++ file extensions
find . -type f \( \
    -name "*.cpp" -o \
    -name "*.hpp" -o \
    -name "*.cc" -o \
    -name "*.cxx" -o \
    -name "*.c++" -o \
    -name "*.h" -o \
    -name "*.hh" -o \
    -name "*.hxx" -o \
    -name "*.h++" \
\) \
-not -path "./build*" \
-not -path "./.git/*" \
-not -path "./third_party/*" \
-not -path "./external/*" \
-exec clang-format -i {} +

if [ $? -eq 0 ]; then
    echo "✅ Formatting completed successfully!"
    echo "All C++ source and header files have been formatted according to .clang-format"
else
    echo "❌ Formatting failed!"
    exit 1
fi

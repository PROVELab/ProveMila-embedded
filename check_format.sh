#!/bin/bash

# Assert clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "clang-format could not be found in your PATH"
    echo "Please install clang-format and try again"
    exit 1
fi

# Find files
files=$(find src/ include/ -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp")

# Check formatting (without modifying files)
if ! echo "$files" | xargs clang-format --dry-run -Werror; then
    echo "Formatting issues detected! Please run ./format.sh to fix them."
    exit 1  # Ensures GitHub Actions fails on error
fi

echo "All files are properly formatted."

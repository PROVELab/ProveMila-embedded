# Assert clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "clang-format could not be found in your PATH"
    echo "Please install clang-format and try again"
    exit 1
fi

# Check all h/hpp/c/cpp files in the project, with -Werror and --dry-run
find src/ -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" | xargs clang-format -i -Werror --dry-run
find include/ -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" | xargs clang-format -i -Werror --dry-run

echo "Checked all files in the project"

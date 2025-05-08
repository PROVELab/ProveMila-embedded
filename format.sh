# Assert clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "clang-format could not be found in your PATH"
    echo "Please install clang-format and try again"
    exit 1
fi

# Format all h/hpp/c/cpp files in the project
find src/ -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" | xargs clang-format -i
find include/ -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" | xargs clang-format -i

echo "Formatted all files in the project"

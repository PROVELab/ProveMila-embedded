#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Make sure the hooks directory exists
mkdir -p hooks

# Make the pre-commit hook executable
chmod +x hooks/pre-commit

# Get the git hooks directory
HOOKS_DIR=$(git rev-parse --git-path hooks)

# Create a symbolic link from the git hooks directory to our pre-commit hook
if [ -f "$HOOKS_DIR/pre-commit" ]; then
    echo -e "${YELLOW}A pre-commit hook already exists. Backing it up.${NC}"
    mv "$HOOKS_DIR/pre-commit" "$HOOKS_DIR/pre-commit.backup"
fi

# Create symbolic link
ln -sf "$(pwd)/hooks/pre-commit" "$HOOKS_DIR/pre-commit"

echo -e "${GREEN}Git pre-commit hook installed successfully!${NC}"
echo -e "${GREEN}The hook will automatically format your code before each commit.${NC}"
echo -e "${YELLOW}Note: If you don't have clang-format installed, the hook will warn you but allow commits to proceed.${NC}"

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${YELLOW}Warning: clang-format is not installed on your system.${NC}"
    echo -e "${YELLOW}To install clang-format:${NC}"
    echo -e "${YELLOW}  - macOS: brew install clang-format${NC}"
    echo -e "${YELLOW}  - Ubuntu/Debian: sudo apt-get install clang-format${NC}"
    echo -e "${YELLOW}  - Windows with Chocolatey: choco install llvm${NC}"
fi 
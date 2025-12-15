#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

CLANG_FORMAT_BIN="${CLANG_FORMAT:-clang-format}"

if ! command -v "$CLANG_FORMAT_BIN" &> /dev/null; then
    echo "clang-format could not be found in your PATH (looked for \"$CLANG_FORMAT_BIN\")."
    echo "Please install clang-format (v21.1.2 to match CI) or set CLANG_FORMAT to the binary to use."
    exit 1
fi

declare -a PATHS
PATHS=(
    'include/**/*.h'
    'include/**/*.hpp'
    'src/**/*.c'
    'src/**/*.cpp'
    'src/**/*.h'
    'src/**/*.hpp'
)

mapfile -t FILES < <(git ls-files -- "${PATHS[@]}" | sort -u)

if [[ ${#FILES[@]} -eq 0 ]]; then
    echo "No C/C++ files found to format."
    exit 0
fi

CLANG_FORMAT_VERSION="$("$CLANG_FORMAT_BIN" --version)"
CURRENT_VERSION="$(grep -Eo '[0-9]+\.[0-9]+\.[0-9]+' <<< "$CLANG_FORMAT_VERSION" | head -n1 || true)"
REQUIRED_VERSION="21.1.2"

if [[ -z "$CURRENT_VERSION" ]]; then
    echo "Unable to determine clang-format version from output: ${CLANG_FORMAT_VERSION}" >&2
    exit 1
fi

if [[ "$CURRENT_VERSION" != "$REQUIRED_VERSION" ]]; then
    echo "Detected clang-format $CURRENT_VERSION, but CI requires $REQUIRED_VERSION." >&2
    echo "Install the matching version (e.g. 'uv tool install \"clang-format==${REQUIRED_VERSION}\"')" >&2
    echo "or set CLANG_FORMAT to point to a clang-format ${REQUIRED_VERSION} binary." >&2
    exit 1
fi

echo "Formatting ${#FILES[@]} file(s) using ${CLANG_FORMAT_VERSION}:"
printf '  %s\n' "${FILES[@]}"

"$CLANG_FORMAT_BIN" -i "${FILES[@]}"

echo "Formatting complete."

#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel 2>/dev/null || echo "${SCRIPT_DIR}")"
cd "${REPO_ROOT}"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format could not be found in your PATH."
    echo "Please install clang-format and try again."
    exit 1
fi

declare -a files=()

SEARCH_DIRS=(src include)

gather_from_dirs() {
    local dir
    for dir in "$@"; do
        if [[ -d "${dir}" ]]; then
            find "${dir}" -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print
        fi
    done
}

already_added() {
    local candidate="$1"
    local existing
    for existing in "${files[@]}"; do
        if [[ "${existing}" == "${candidate}" ]]; then
            return 0
        fi
    done
    return 1
}

filter_files() {
    local path
    for path in "$@"; do
        if [[ -n "${path}" && -f "${path}" ]]; then
            case "${path}" in
                *.c|*.cpp|*.h|*.hpp)
                    if ! already_added "${path}"; then
                        files+=("${path}")
                    fi
                    ;;
            esac
        fi
    done
}

if [[ $# -gt 0 ]]; then
    filter_files "$@"
else
    while IFS= read -r path; do
        filter_files "${path}"
    done < <(gather_from_dirs "${SEARCH_DIRS[@]}")
fi

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No C/C++ files found to check."
    exit 0
fi

echo "Checking formatting for ${#files[@]} file(s)."

if ! clang-format --dry-run -Werror "${files[@]}"; then
    echo "Formatting issues detected! Please run ./format.sh to fix them."
    exit 1
fi

echo "All checked files are properly formatted."

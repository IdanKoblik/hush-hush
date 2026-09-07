#!/usr/bin/env bash
set -euo pipefail

CLANG_FORMAT_VERSION='22.1.8'
CLANG_FORMAT=${CLANG_FORMAT:-clang-format}

check_only=0
case "${1-}" in
    --check) check_only=1 ;;
    "") ;;
    *)
        echo "usage: $0 [--check]" >&2
        echo "  --check  report unformatted files without rewriting them" >&2
        exit 64
        ;;
esac

cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

if ! command -v "$CLANG_FORMAT" >/dev/null 2>&1; then
    echo "$0: $CLANG_FORMAT not found (pip install 'clang-format==$CLANG_FORMAT_VERSION')" >&2
    exit 127
fi

version=$("$CLANG_FORMAT" --version)
if [[ "$version" != *"$CLANG_FORMAT_VERSION"* ]]; then
    echo "$0: warning: CI pins clang-format $CLANG_FORMAT_VERSION, found: $version" >&2
fi

mapfile -t files < <(git ls-files '*.c' '*.h' '*.cpp' '*.hpp' ':!:third_party/**' ':!:testing/greatest.h')

if [ "${#files[@]}" -eq 0 ]; then
    echo "$0: no sources matched" >&2
    exit 1
fi

if [ "$check_only" -eq 1 ]; then
    unformatted=()
    for file in "${files[@]}"; do
        if ! "$CLANG_FORMAT" --dry-run -Werror "$file" >/dev/null 2>&1; then
            unformatted+=("$file")
        fi
    done

    if [ "${#unformatted[@]}" -ne 0 ]; then
        printf '%s\n' "${unformatted[@]}"
        printf '%d of %d files need formatting; run %s\n' "${#unformatted[@]}" "${#files[@]}" "$0" >&2
        exit 1
    fi

    printf 'All %d files match .clang-format\n' "${#files[@]}"
    exit 0
fi

"$CLANG_FORMAT" -i "${files[@]}"
printf 'Formatted %d files\n' "${#files[@]}"

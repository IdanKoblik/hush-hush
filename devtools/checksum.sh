#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 2 ]; then
    echo "usage: $0 <file_a> <file_b>" >&2
    exit 64
fi

sha_a=$(sha256sum "$1" | cut -d' ' -f1)
sha_b=$(sha256sum "$2" | cut -d' ' -f1)

if [ "$sha_a" = "$sha_b" ]; then
    echo "identical ($sha_a)"
    exit 0
else
    echo "$1: $sha_a"
    echo "$2: $sha_b"
    exit 1
fi

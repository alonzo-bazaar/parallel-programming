#!/usr/bin/env sh
SCRIPT_DIR="$(dirname "$0")"
echo "in dir ${SCRIPT_DIR}..."
clang -Wall -Wextra -Wpedantic -Werror -g -O2 -o "${SCRIPT_DIR}/shsl" "${SCRIPT_DIR}/shsl.c"

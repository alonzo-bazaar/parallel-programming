#!/usr/bin/env bash

# https://github.com/dylanaraps/pure-bash-bible#replacement
# execute all following commands in the directory where the script resides
cd "${0%/*}" || (echo "wait what the fuck" && exit 1)

clang -o shsl shsl.c

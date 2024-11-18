#!/bin/sh

set -xe

find . -name "*.c" -o -name "*.h" | xargs clang-format -i
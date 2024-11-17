#!/bin/sh

set -xe

FLAGS="-std=gnu11"
DEV_FLAGS="-Wall -Wextra -Wpedantic -Werror"
DEV_FLAGS+=" -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function"
DEV_FLAGS+=" -g3 -O0 -DDEBUG -D_GLIBC_DEBUG"

gcc $FLAGS $DEV_FLAGS main.c mm.c memlib.c perf.c -o main
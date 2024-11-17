#!/bin/sh

set -xe

FLAGS="-std=gnu11 -lm"
DEV_FLAGS="-Wall -Wextra -Wpedantic -Werror"
DEV_FLAGS+=" -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function"
DEV_FLAGS+=" -g3 -O0 -DDEBUG -D_GLIBC_DEBUG"

SRC="main.c mm.c memlib.c perf.c vec_u64.c"

gcc $FLAGS $DEV_FLAGS $SRC -o main
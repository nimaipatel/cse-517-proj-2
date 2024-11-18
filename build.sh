#!/bin/sh

set -xe

CC="gcc"

FLAGS="-std=gnu11 -lm"

DEV_FLAGS="-Wall -Wextra -Wpedantic -Werror"
DEV_FLAGS+=" -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function"
DEV_FLAGS+=" -g3 -O0 -DDEBUG -D_GLIBC_DEBUG"

RELEASE_FLAGS="-O3 -DNDEBUG"

SRC="main.c mm.c memlib.c perf.c vec_u64.c"

if [ "$1" = "debug" ]; then
    $CC $FLAGS $DEV_FLAGS $SRC -o main
else
    $CC $FLAGS $RELEASE_FLAGS $SRC -o main
fi
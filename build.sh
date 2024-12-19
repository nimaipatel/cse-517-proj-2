#!/bin/sh

set -xe

CC="gcc"

FLAGS="-std=gnu11 -lm"
FLAGS+=" -Wall -Wextra -Wpedantic -Werror"
FLAGS+=" -Wdouble-promotion -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function"

DEV_FLAGS="-g3 -O0 -DDEBUG -D_GLIBC_DEBUG"

RELEASE_FLAGS="-O3 -DNDEBUG"

SRC="main.c"
SRC+=" trace.c"
SRC+=" trace_parser.c"
SRC+=" mm.c"
SRC+=" heapsim.c"
SRC+=" perf.c"
SRC+=" vec_u64.c"
SRC+=" string.c"
SRC+=" csv.c"

if [ "$1" = "debug" ]; then
    $CC $FLAGS $DEV_FLAGS $SRC -o main
elif [ "$1" = "release" ]; then
    $CC $FLAGS $RELEASE_FLAGS $SRC -o main
else
    echo "Unknown build type"
    exit 1
fi

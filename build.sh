#!/bin/sh

set -xe

FLAGS="-std=gnu11"
DEV_FLAGS="-Wall -Wextra -Wpedantic -Werror\
  -Wdouble-promotion\
 -Wno-unused-parameter -Wno-unused-function\
 -g3 -O0"

gcc $FLAGS $DEV_FLAGS main.c mm.c memlib.c -o main
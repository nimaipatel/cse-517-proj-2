#ifndef _VEC_U64_H
#define _VEC_U64_H

#include <stddef.h>
#include <stdint.h>

typedef struct Vec_U64 {
#define VEC_INIT_CAP 0x10
    size_t len;
    size_t cap;
    uint64_t *data;
} Vec_U64;

typedef struct Vec_U64_Stats_Result {
    double mean;
    double variance;
    double stddev;
    double margin_of_error;
} Vec_U64_Stats_Result;

void Vec_U64_Push(Vec_U64 *, uint64_t);
void Vec_U64_Free(Vec_U64 *);
Vec_U64_Stats_Result Vec_U64_Stats(Vec_U64);

#endif // _VEC_U64_H

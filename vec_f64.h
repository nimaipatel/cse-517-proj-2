#include "defines.h"

#ifndef VEC_F64_H
#define VEC_F64_H

typedef struct Vec_F64 {
#define VEC_INIT_CAP 0x10
    F64 *data;
    size_t len;
    size_t cap;
} Vec_F64;

typedef struct Vec_F64_Stats_Result {
    double mean;
    double variance;
    double stddev;
    double margin_of_error;
} Vec_F64_Stats_Result;

void Vec_F64_Release(Vec_F64 *vec);
void Vec_F64_Push(Vec_F64 *vec, F64 value);
Vec_F64_Stats_Result Vec_F64_Stats(Vec_F64 *vec);

#endif // VEC_F64_H

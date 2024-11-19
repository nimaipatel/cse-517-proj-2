#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "vec_u64.h"
#include "defines.h"

void
Vec_U64_Push(Vec_U64 *vec, uint64_t val)
{
    if (vec->len == vec->cap) {
        vec->cap = MAX(vec->cap * 2, VEC_INIT_CAP);
        vec->data = realloc(vec->data, vec->cap * sizeof(*vec->data));
        assert(vec->data);
    }
    vec->data[vec->len++] = val;
}

void
Vec_U64_Reserve(Vec_U64 *vec, size_t cap)
{
    vec->cap = MAX(vec->cap, cap);
    vec->data = realloc(vec->data, vec->cap * sizeof(*vec->data));
}

void
__Vec_U64_Append(Vec_U64 *vec, size_t num_others, ...)
{
    size_t new_len = vec->len;

    va_list args;
    va_start(args, num_others);

    for (size_t i = 0; i < num_others; i++) {
        Vec_U64 other = va_arg(args, Vec_U64);
        new_len += other.len;
    }

    va_end(args);

    Vec_U64_Reserve(vec, new_len);

    va_start(args, num_others);
    for (size_t i = 0; i < num_others; i++) {
        Vec_U64 other = va_arg(args, Vec_U64);
        for (size_t j = 0; j < other.len; j++) {
            vec->data[vec->len++] = other.data[j];
        }
    }

    va_end(args);
}

void
Vec_U64_Release(Vec_U64 vec)
{
    free(vec.data);
}

Vec_U64_Stats_Result
Vec_U64_Stats(Vec_U64 vec)
{
    if (vec.len == 0) {
        return (Vec_U64_Stats_Result){ 0 };
    }

    uint64_t sum = 0;
    for (size_t i = 0; i < vec.len; i += 1) {
        sum += vec.data[i];
    }
    double mean = sum / vec.len;

    double variance = 0;
    for (size_t i = 0; i < vec.len; i += 1) {
        const double diff = vec.data[i] - mean;
        variance += diff * diff;
    }
    variance /= vec.len;

    double stddev = sqrt(variance);

    double zstar = 1.96; // large sample size, 95% confidence
    double margin_of_error = zstar * stddev / sqrt(vec.len);

    return (Vec_U64_Stats_Result){
        .sum = sum,
        .mean = mean,
        .variance = variance,
        .stddev = stddev,
        .margin_of_error = margin_of_error,
    };
}
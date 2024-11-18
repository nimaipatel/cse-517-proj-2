#include <math.h>
#include <stdlib.h>

#include "defines.h"
#include "vec_f64.h"

void
Vec_F64_Release(Vec_F64 *vec)
{
    free(vec->data);
}

void
Vec_F64_Push(Vec_F64 *vec, F64 value)
{
    if (vec->len == vec->cap) {
        vec->cap = MAX(vec->cap * 2, VEC_INIT_CAP);
        vec->data = realloc(vec->data, vec->cap * sizeof(*vec->data));
    }
    vec->data[vec->len] = value;
    vec->len += 1;
}

Vec_F64_Stats_Result
Vec_F64_Stats(Vec_F64 *vec)
{
    if (vec->len == 0) {
        return (Vec_F64_Stats_Result){ 0 };
    }

    F64 mean = 0;
    for (size_t i = 0; i < vec->len; i += 1) {
        mean += vec->data[i];
    }
    mean /= vec->len;

    F64 variance = 0;
    for (size_t i = 0; i < vec->len; i += 1) {
        variance += (vec->data[i] - mean) * (vec->data[i] - mean);
    }
    variance /= vec->len;

    F64 stddev = sqrt(variance);

    const F64 zstar = 1.96;
    F64 margin_of_error = zstar * stddev / sqrt(vec->len);

    return (Vec_F64_Stats_Result){
        .mean = mean,
        .variance = variance,
        .stddev = stddev,
        .margin_of_error = margin_of_error,
    };
}

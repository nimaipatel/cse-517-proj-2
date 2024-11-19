#ifndef _TRACE_H
#define _TRACE_H

#include "defines.h"
#include "vec_u64.h"

typedef struct Trace_Op {
    enum { ALLOC, FREE, REALLOC } type;
    size_t id;
    size_t size;
} Trace_Op;

typedef struct Trace {
    size_t num_ids;
    size_t num_ops;
    size_t data_bytes;
    Trace_Op *ops;
} Trace;

typedef struct Trace_Run_Result {
    Vec_U64 malloc_cyc;
    Vec_U64 realloc_cyc;
    Vec_U64 free_cyc;
    F64 util;
} Trace_Run_Result;

Trace_Run_Result Trace_Run(Trace, U64 perf_type, U64 perf_config);

#endif // _TRACE_H

#ifndef _TRACE_H
#define _TRACE_H

#include "defines.h"
#include "vec_u64.h"

typedef struct Trace_Op {
    enum { ALLOC, FREE, REALLOC } type;
    size_t index;
    size_t size;
} Trace_Op;

typedef struct Trace {
    size_t num_ids;
    size_t num_ops;
    size_t data_bytes;
    Trace_Op *ops;
} Trace;

struct Trace_Run_Result {
    Vec_U64 malloc_inst;
    Vec_U64 realloc_inst;
    Vec_U64 free_inst;
};

struct Trace_Run_Result Trace_Run(Trace);

#endif // _TRACE_H

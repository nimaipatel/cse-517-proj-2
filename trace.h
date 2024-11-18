#ifndef _TRACE_H
#define _TRACE_H

#include "defines.h"
#include "string_view.h"

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

Trace Trace_Parse(String_View input);

#endif // _TRACE_H
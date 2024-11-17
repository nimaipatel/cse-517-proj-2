#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <linux/perf_event.h>

#include "mm.h"
#include "memlib.h"
#include "perf.h"

static const char *traces[] = {
    "traces/bdd-aa32.rep",         "traces/bdd-aa4.rep",
    "traces/bdd-ma4.rep",          "traces/bdd-nq7.rep",
    "traces/cbit-abs.rep",         "traces/cbit-parity.rep",
    "traces/cbit-satadd.rep",      "traces/cbit-xyz.rep",
    "traces/ngram-fox1.rep",       "traces/ngram-gulliver1.rep",
    "traces/ngram-gulliver2.rep",  "traces/ngram-moby1.rep",
    "traces/ngram-shake1.rep",     "traces/syn-array.rep",
    "traces/syn-array-short.rep",  "traces/syn-largemem-short.rep",
    "traces/syn-mix-realloc.rep",  "traces/syn-mix.rep",
    "traces/syn-mix-short.rep",    "traces/syn-string.rep",
    "traces/syn-string-short.rep", "traces/syn-struct.rep",
    "traces/syn-struct-short.rep",
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

static Trace
Trace_Read(const char *filename)
{
    Trace trace = { 0 };

    FILE *tracefile = fopen(filename, "r");
    assert(tracefile && "Failed to open trace file");

    size_t iweight;
    fscanf(tracefile, "%zu", &iweight);
    fscanf(tracefile, "%zu", &trace.num_ids);
    fscanf(tracefile, "%zu", &trace.num_ops);
    fscanf(tracefile, "%zu", &trace.data_bytes);

    trace.ops = malloc(trace.num_ops * sizeof(trace.ops[0]));
    assert(trace.ops && "Allocation Failure");

    char type[1024];
    size_t op_index = 0;
    size_t max_id = 0;
    while (fscanf(tracefile, "%s", type) != EOF) {
        switch (type[0]) {
        case 'a': {
            size_t id;
            size_t alloc_size;
            fscanf(tracefile, "%zu %lu", &id, &alloc_size);
            trace.ops[op_index] = (Trace_Op){ ALLOC, id, alloc_size };
            max_id = MAX(id, max_id);
            break;
        }

        case 'r': {
            size_t id;
            size_t alloc_size;
            fscanf(tracefile, "%zu %lu", &id, &alloc_size);
            trace.ops[op_index] = (Trace_Op){ REALLOC, id, alloc_size };
            max_id = MAX(id, max_id);
            break;
        }

        case 'f': {
            size_t id;
            fscanf(tracefile, "%zu", &id);
            trace.ops[op_index] = (Trace_Op){ FREE, id, 0 };
            max_id = MAX(id, max_id);
            break;
        }

        default: {
            assert(false && "Unknown trace operation");
        }
        }

        op_index += 1;
        if (op_index == trace.num_ops) {
            break;
        }
    }

    fclose(tracefile);
    assert(max_id == trace.num_ids - 1);
    assert(trace.num_ops == op_index);

    return trace;
}

static bool
Trace_Run(Trace trace)
{
    mem_reset_brk();

    if (!mm_init()) {
        return false;
    }

    void **alloc_ptrs = malloc(trace.num_ids * sizeof(*alloc_ptrs));
    assert(alloc_ptrs);

    size_t *alloc_sizes = malloc(trace.num_ids * sizeof(*alloc_sizes));
    assert(alloc_sizes);

    for (size_t i = 0; i < trace.num_ops; i += 1) {
        size_t index = trace.ops[i].index;
        size_t size = trace.ops[i].size;

        switch (trace.ops[i].type) {
        case ALLOC: {
            alloc_ptrs[index] = mm_malloc(size);
            alloc_sizes[index] = size;
            break;
        }

        case REALLOC: {
            alloc_ptrs[index] = mm_realloc(alloc_ptrs[index], size);
            alloc_sizes[index] = size;
            break;
        }

        case FREE: {
            mm_free(alloc_ptrs[index]);
            break;
        }

        default: {
            assert(false && "Unknown trace operation");
        }
        }
    }
    return true;
}

int
main(void)
{
    mem_init();

    for (size_t i = 0; i < sizeof(traces) / sizeof(*traces); i += 1) {
        Trace trace = Trace_Read(traces[i]);
        Trace_Run(trace);
    }

    mem_deinit();
    return 0;
}

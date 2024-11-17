#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <linux/perf_event.h>

#include "mm.h"
#include "memlib.h"
#include "perf.h"
#include "vec_u64.h"

static const char *traces[] = {
    "traces/bdd-aa32.rep", "traces/bdd-aa4.rep",
    // "traces/bdd-ma4.rep",          "traces/bdd-nq7.rep",
    // "traces/cbit-abs.rep",         "traces/cbit-parity.rep",
    // "traces/cbit-satadd.rep",      "traces/cbit-xyz.rep",
    // "traces/ngram-fox1.rep",       "traces/ngram-gulliver1.rep",
    // "traces/ngram-gulliver2.rep",  "traces/ngram-moby1.rep",
    // "traces/ngram-shake1.rep",     "traces/syn-array.rep",
    // "traces/syn-array-short.rep",  "traces/syn-largemem-short.rep",
    // "traces/syn-mix-realloc.rep",  "traces/syn-mix.rep",
    // "traces/syn-mix-short.rep",    "traces/syn-string.rep",
    // "traces/syn-string-short.rep", "traces/syn-struct.rep",
    // "traces/syn-struct-short.rep",
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

struct Trace_Run_Result {
    Vec_U64_Stats_Result malloc;
    Vec_U64_Stats_Result realloc;
    Vec_U64_Stats_Result free;
    bool valid;
};

static struct Trace_Run_Result
Trace_Run(Trace trace)
{
    mem_reset_brk();

    if (!mm_init()) {
        return (struct Trace_Run_Result){ 0 };
    }

    void **alloc_ptrs = malloc(trace.num_ids * sizeof(*alloc_ptrs));
    assert(alloc_ptrs);

    size_t *alloc_sizes = malloc(trace.num_ids * sizeof(*alloc_sizes));
    assert(alloc_sizes);

    Vec_U64 malloc_cycles = { 0 };
    Vec_U64 realloc_cycles = { 0 };
    Vec_U64 free_cycles = { 0 };

    // TODO: check success of malloc, realloc, free...
    for (size_t i = 0; i < trace.num_ops; i += 1) {
        size_t index = trace.ops[i].index;
        size_t size = trace.ops[i].size;

        switch (trace.ops[i].type) {
        case ALLOC: {
            int fd = Perf_Start(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
            alloc_ptrs[index] = mm_malloc(size);
            uint64_t cycles = Perf_Stop(fd);
            Vec_U64_Push(&malloc_cycles, cycles);
            break;
        }

        case REALLOC: {
            int fd = Perf_Start(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
            alloc_ptrs[index] = mm_realloc(alloc_ptrs[index], size);
            uint64_t cycles = Perf_Stop(fd);
            Vec_U64_Push(&realloc_cycles, cycles);
            break;
        }

        case FREE: {
            int fd = Perf_Start(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
            mm_free(alloc_ptrs[index]);
            uint64_t cycles = Perf_Stop(fd);
            Vec_U64_Push(&free_cycles, cycles);
            break;
        }
        default: {
            assert(false && "Unknown trace operation");
        }
        }
    }

    assert(malloc_cycles.len + realloc_cycles.len + free_cycles.len ==
           trace.num_ops);

    Vec_U64_Stats_Result malloc_stats = Vec_U64_Stats(malloc_cycles);
    Vec_U64_Stats_Result realloc_stats = Vec_U64_Stats(realloc_cycles);
    Vec_U64_Stats_Result free_stats = Vec_U64_Stats(free_cycles);

    Vec_U64_Free(&malloc_cycles);
    Vec_U64_Free(&realloc_cycles);
    Vec_U64_Free(&free_cycles);

    return (struct Trace_Run_Result){
        .malloc = malloc_stats,
        .realloc = realloc_stats,
        .free = free_stats,
        .valid = true,
    };
}

int
main(void)
{
    mem_init();

    for (size_t i = 0; i < sizeof(traces) / sizeof(*traces); i += 1) {
        Trace trace = Trace_Read(traces[i]);
        struct Trace_Run_Result result = Trace_Run(trace);
        printf("%s:\n", traces[i]);
        printf("malloc: %f ± %f\n", result.malloc.mean,
               result.malloc.margin_of_error);
        printf("realloc: %f ± %f\n", result.realloc.mean,
               result.realloc.margin_of_error);
        printf("free: %f ± %f\n", result.free.mean,
               result.free.margin_of_error);
        printf("\n");
    }

    mem_deinit();
    return 0;
}

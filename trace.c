#include <linux/perf_event.h>
#include <assert.h>

#include "defines.h"
#include "vec_u64.h"
#include "trace.h"
#include "memlib.h"
#include "mm.h"
#include "perf.h"
#include "trace.h"

struct Trace_Run_Result
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

    Vec_U64 malloc_inst = { 0 };
    Vec_U64 realloc_inst = { 0 };
    Vec_U64 free_inst = { 0 };

    // TODO: check success of malloc, realloc, free...
    for (size_t i = 0; i < trace.num_ops; i += 1) {
        size_t index = trace.ops[i].index;
        size_t size = trace.ops[i].size;

        switch (trace.ops[i].type) {
        case ALLOC: {
            int fd = Perf_Start(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
            alloc_ptrs[index] = mm_malloc(size);
            U64 cycles = Perf_Stop(fd);
            Vec_U64_Push(&malloc_inst, cycles);
            break;
        }

        case REALLOC: {
            int fd = Perf_Start(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
            alloc_ptrs[index] = mm_realloc(alloc_ptrs[index], size);
            U64 cycles = Perf_Stop(fd);
            Vec_U64_Push(&realloc_inst, cycles);
            break;
        }

        case FREE: {
            int fd = Perf_Start(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
            mm_free(alloc_ptrs[index]);
            U64 cycles = Perf_Stop(fd);
            Vec_U64_Push(&free_inst, cycles);
            break;
        }
        default: {
            assert(false && "Unknown trace operation");
        }
        }
    }

    assert(malloc_inst.len + realloc_inst.len + free_inst.len == trace.num_ops);

    free(alloc_ptrs);
    free(alloc_sizes);

    return (struct Trace_Run_Result){
        .malloc_inst = malloc_inst,
        .realloc_inst = realloc_inst,
        .free_inst = free_inst,
    };
}

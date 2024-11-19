#include <stdio.h>
#include <linux/perf_event.h>
#include <assert.h>

#include "defines.h"
#include "vec_u64.h"
#include "trace.h"
#include "memlib.h"
#include "mm.h"
#include "perf.h"
#include "trace.h"

Trace_Run_Result
Trace_Run(Trace trace, U64 perf_type, U64 perf_config)
{
    Heap_Sim_Brk();

    if (!M_Init()) {
        fprintf(stderr, "M_Init failed\n");
        exit(1);
    }

    void **alloc_ptrs = malloc(trace.num_ids * sizeof(*alloc_ptrs));
    if (alloc_ptrs == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }

    size_t *alloc_sizes = malloc(trace.num_ids * sizeof(*alloc_sizes));
    if (alloc_sizes == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }

    Vec_U64 malloc_cyc = { 0 };
    Vec_U64 realloc_cyc = { 0 };
    Vec_U64 free_cyc = { 0 };

    U64 total_alloc_size = 0;
    U64 max_alloc_size = 0;
    U64 max_heap_size = Heap_Sim_Get_Heap_Size();

    for (size_t i = 0; i < trace.num_ops; i += 1) {
        size_t id = trace.ops[i].id;
        size_t size = trace.ops[i].size;

        switch (trace.ops[i].type) {
        case ALLOC: {
            int fd = Perf_Start(perf_type, perf_config);
            // TODO: check success of malloc, realloc, free...
            void *ptr = M_malloc(size);
            U64 cycles = Perf_Stop(fd);

            total_alloc_size += size;
            alloc_ptrs[id] = ptr;
            alloc_sizes[id] = size;
            Vec_U64_Push(&malloc_cyc, cycles);
            break;
        }

        case REALLOC: {
            int fd = Perf_Start(perf_type, perf_config);
            void *ptr = M_realloc(alloc_ptrs[id], size);
            U64 cycles = Perf_Stop(fd);

            total_alloc_size += size - alloc_sizes[id];
            alloc_ptrs[id] = ptr;
            alloc_sizes[id] = size;
            Vec_U64_Push(&realloc_cyc, cycles);
            break;
        }

        case FREE: {
            int fd = Perf_Start(perf_type, perf_config);
            M_free(alloc_ptrs[id]);
            U64 cycles = Perf_Stop(fd);

            total_alloc_size -= alloc_sizes[id];
            Vec_U64_Push(&free_cyc, cycles);
            break;
        }
        default: {
            assert(false && "Unknown trace operation");
        }
        }

        max_alloc_size = MAX(max_alloc_size, total_alloc_size);
        max_heap_size = MAX(max_heap_size, Heap_Sim_Get_Heap_Size());
    }

    assert(malloc_cyc.len + realloc_cyc.len + free_cyc.len == trace.num_ops);

    free(alloc_ptrs);
    free(alloc_sizes);

    return (Trace_Run_Result){
        .malloc_cyc = malloc_cyc,
        .realloc_cyc = realloc_cyc,
        .free_cyc = free_cyc,
        .util = (double)max_alloc_size / (double)max_heap_size,
    };
}

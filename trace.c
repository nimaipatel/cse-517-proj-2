/*
 * Copyright (C) 2024 Patel, Nimai <nimai.m.patel@gmail.com>
 * Author: Patel, Nimai <nimai.m.patel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <linux/perf_event.h>
#include <assert.h>

#include "defines.h"
#include "vec_u64.h"
#include "trace.h"
#include "heapsim.h"
#include "mm.h"
#include "perf.h"
#include "trace.h"

Trace_Run_Result
Trace_Run(Trace trace, U64 perf_type, U64 perf_config)
{
    Heap_Sim_Brk();

    if (!M_Init())
    {
        fprintf(stderr, "M_Init failed\n");
        exit(1);
    }

    void **alloc_ptrs;
    size_t *alloc_sizes;
    U8 *_ = malloc(trace.num_ids * sizeof(*alloc_ptrs) + trace.num_ids * sizeof(*alloc_sizes));
    if (!_)
    {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    alloc_ptrs = (void **)_;
    alloc_sizes = (size_t *)(_ + trace.num_ids * sizeof(*alloc_ptrs));

    Vec_U64 malloc_cyc = { 0 };
    Vec_U64 realloc_cyc = { 0 };
    Vec_U64 free_cyc = { 0 };

    U64 total_alloc_size = 0;
    U64 max_alloc_size = 0;
    U64 max_heap_size = Heap_Sim_Get_Heap_Size();

    for (size_t i = 0; i < trace.num_ops; i += 1)
    {
        size_t id = trace.ops[i].id;
        size_t size = trace.ops[i].size;

        switch (trace.ops[i].type)
        {
        case ALLOC:
        {
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

        case REALLOC:
        {
            int fd = Perf_Start(perf_type, perf_config);
            void *ptr = M_realloc(alloc_ptrs[id], size);
            U64 cycles = Perf_Stop(fd);

            total_alloc_size += size - alloc_sizes[id];
            alloc_ptrs[id] = ptr;
            alloc_sizes[id] = size;
            Vec_U64_Push(&realloc_cyc, cycles);
            break;
        }

        case FREE:
        {
            int fd = Perf_Start(perf_type, perf_config);
            M_free(alloc_ptrs[id]);
            U64 cycles = Perf_Stop(fd);

            total_alloc_size -= alloc_sizes[id];
            Vec_U64_Push(&free_cyc, cycles);
            break;
        }
        default:
        {
            assert(false && "Unknown trace operation");
        }
        }

        max_alloc_size = MAX(max_alloc_size, total_alloc_size);
        max_heap_size = MAX(max_heap_size, Heap_Sim_Get_Heap_Size());
    }

    assert(malloc_cyc.len + realloc_cyc.len + free_cyc.len == trace.num_ops);

    free(_);

    return (Trace_Run_Result){
        .malloc_cyc = malloc_cyc,
        .realloc_cyc = realloc_cyc,
        .free_cyc = free_cyc,
        .util = (double)max_alloc_size / (double)max_heap_size,
    };
}

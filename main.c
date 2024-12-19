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
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <linux/perf_event.h>

#include "heapsim.h"
#include "string.h"
#include "trace_parser.h"
#include "trace.h"
#include "csv.h"
#include "config.h"

static Char8 *traces[] = {
    // These are traces from CMU malloc lab, used in this project with
    // permission from professor Randy Bryant and Professor David O'Hallaron

    "traces/bdd-aa32.rep", "traces/bdd-aa4.rep", "traces/bdd-ma4.rep", "traces/bdd-nq7.rep", "traces/cbit-abs.rep",
    "traces/cbit-parity.rep", "traces/cbit-satadd.rep", "traces/cbit-xyz.rep", "traces/ngram-fox1.rep",
    "traces/ngram-gulliver1.rep", "traces/ngram-gulliver2.rep", "traces/ngram-moby1.rep", "traces/ngram-shake1.rep",
    "traces/syn-array.rep", "traces/syn-array-short.rep", "traces/syn-largemem-short.rep", "traces/syn-mix-realloc.rep",
    "traces/syn-mix.rep", "traces/syn-mix-short.rep", "traces/syn-string.rep", "traces/syn-string-short.rep",
    "traces/syn-struct.rep", "traces/syn-struct-short.rep",

    // trace for lox interpreter from
    // https://github.com/munificent/craftinginterpreters/ running
    // test/benchmark/trees.lox refer readme.txt to see how to run generate
    // trace for your own program
    "traces/clox-trees-lox.rep"
};
#define NUM_TRACES (sizeof(traces) / sizeof(*traces))

int
main(void)
{
    Heap_Sim_Init();

    Vec_U64 malloc_cyc = { 0 };
    Vec_U64 realloc_cyc = { 0 };
    Vec_U64 free_cyc = { 0 };

    double util_sum = 0;

    FILE *f = CSV_Open(RUN_NAME ".csv");
    CSV_Write_Header(f);

    for (size_t i = 0; i < NUM_TRACES; i += 1)
    {
        String input = String_Read_File(traces[i]);
        Trace trace = Trace_Parse(String_Slice(input, 0, input.len));

        Trace_Run_Result result = Trace_Run(trace, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);

        Vec_U64 overall = { 0 };
        Vec_U64_Append(&overall, result.malloc_cyc, result.realloc_cyc, result.free_cyc);

        Vec_U64_Stats_Result malloc = Vec_U64_Stats(result.malloc_cyc);
        Vec_U64_Stats_Result realloc = Vec_U64_Stats(result.realloc_cyc);
        Vec_U64_Stats_Result free = Vec_U64_Stats(result.free_cyc);
        Vec_U64_Stats_Result total = Vec_U64_Stats(overall);

        CSV_Write(f, basename(traces[i]), malloc.mean, malloc.margin_of_error, realloc.mean, realloc.margin_of_error,
                  free.mean, free.margin_of_error, total.mean, total.margin_of_error, result.util);

        util_sum += result.util;
        Vec_U64_Append(&malloc_cyc, result.malloc_cyc);
        Vec_U64_Append(&realloc_cyc, result.realloc_cyc);
        Vec_U64_Append(&free_cyc, result.free_cyc);

        // loop_clean_up
        Vec_U64_Release(result.malloc_cyc);
        Vec_U64_Release(result.realloc_cyc);
        Vec_U64_Release(result.free_cyc);
        Vec_U64_Release(overall);

        Trace_Release(trace);
        String_Release(input);
    }

    Vec_U64 overall = { 0 };
    Vec_U64_Append(&overall, malloc_cyc, realloc_cyc, free_cyc);

    Vec_U64_Stats_Result malloc = Vec_U64_Stats(malloc_cyc);
    Vec_U64_Stats_Result realloc = Vec_U64_Stats(realloc_cyc);
    Vec_U64_Stats_Result free = Vec_U64_Stats(free_cyc);
    Vec_U64_Stats_Result total = Vec_U64_Stats(overall);
    F64 util = util_sum / NUM_TRACES;

    CSV_Write(f, "All Traces", malloc.mean, malloc.margin_of_error, realloc.mean, realloc.margin_of_error, free.mean,
              free.margin_of_error, total.mean, total.margin_of_error, util);

    Vec_U64_Release(malloc_cyc);
    Vec_U64_Release(realloc_cyc);
    Vec_U64_Release(free_cyc);
    Vec_U64_Release(overall);

    Heap_Sim_Release();
    return 0;
}

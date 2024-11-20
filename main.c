#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <linux/perf_event.h>

#include "memlib.h"
#include "string.h"
#include "trace_parser.h"
#include "trace.h"

static const Char8 *traces[] = {
    /* These are traces from CMU malloc lab, used in this project with
       permission from professor Randy Bryant and Professor David O'Hallaron */
    // "traces/bdd-aa32.rep",         "traces/bdd-aa4.rep",
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

    /* trace for lox interpreter from
       https://github.com/munificent/craftinginterpreters/ running
       test/benchmark/trees.lox */
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

    for (size_t i = 0; i < NUM_TRACES; i += 1) {
        String input = String_Read_File(traces[i]);
        Trace trace = Trace_Parse(String_Slice(input, 0, input.len));

        Trace_Run_Result result = Trace_Run(trace, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);

        printf("Trace: %s\n", traces[i]);

        Vec_U64_Stats_Result stats;
        stats = Vec_U64_Stats(result.malloc_cyc);
        printf("malloc:  %f ± %f\n", stats.mean, stats.margin_of_error);

        stats = Vec_U64_Stats(result.realloc_cyc);
        printf("realloc: %f ± %f\n", stats.mean, stats.margin_of_error);

        stats = Vec_U64_Stats(result.free_cyc);
        printf("free:    %f ± %f\n", stats.mean, stats.margin_of_error);

        Vec_U64 overall = { 0 };
        Vec_U64_Append(&overall, result.malloc_cyc, result.realloc_cyc, result.free_cyc);

        stats = Vec_U64_Stats(overall);
        printf("overall: %f ± %f\n", stats.mean, stats.margin_of_error);

        printf("util:    %f\n", result.util);
        util_sum += result.util;

        printf("\n");

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

    printf("All traces:\n");

    printf("malloc:  ");
    Vec_U64_Stats_Result stats = Vec_U64_Stats(malloc_cyc);
    printf("%f ± %f\n", stats.mean, stats.margin_of_error);

    printf("realloc: ");
    stats = Vec_U64_Stats(realloc_cyc);
    printf("%f ± %f\n", stats.mean, stats.margin_of_error);

    printf("free:    ");
    stats = Vec_U64_Stats(free_cyc);
    printf("%f ± %f\n", stats.mean, stats.margin_of_error);

    printf("overall: ");
    stats = Vec_U64_Stats(overall);
    printf("%f ± %f\n", stats.mean, stats.margin_of_error);

    printf("util:    %f\n", util_sum / NUM_TRACES);

    Vec_U64_Release(malloc_cyc);
    Vec_U64_Release(realloc_cyc);
    Vec_U64_Release(free_cyc);
    Vec_U64_Release(overall);

    Heap_Sim_Release();
    return 0;
}

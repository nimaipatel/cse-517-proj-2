#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <linux/perf_event.h>

#include "memlib.h"
#include "string.h"
#include "trace_parser.h"
#include "trace.h"

static const Char8 *traces[] = {
    // These are traces from CMU malloc lab, used in this project with
    // permission from professor Randy Bryant and Professor David O'Hallaron
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
#define NUM_TRACES (sizeof(traces) / sizeof(*traces))

int
main(void)
{
    Heap_Sim_Init();

    for (size_t i = 0; i < NUM_TRACES; i += 1) {
        String input = String_Read_File(traces[i]);
        Trace trace = Trace_Parse(String_Slice(input, 0, input.len));

        Trace_Run_Result result = Trace_Run(trace, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);

        printf("Trace: %s\n", traces[i]);

        Vec_U64_Stats_Result stats = Vec_U64_Stats(result.malloc_cyc);
        printf("malloc:  %f ± %f\n", stats.mean, stats.margin_of_error);

        stats = Vec_U64_Stats(result.realloc_cyc);
        printf("realloc: %f ± %f\n", stats.mean, stats.margin_of_error);

        stats = Vec_U64_Stats(result.free_cyc);
        printf("free:    %f ± %f\n", stats.mean, stats.margin_of_error);

        printf("util:    %f\n", result.util);

        printf("\n");

        Vec_U64_Free(result.malloc_cyc);
        Vec_U64_Free(result.realloc_cyc);
        Vec_U64_Free(result.free_cyc);

        Trace_Release(trace);
        String_Release(input);
    }

    Heap_Sim_Release();
    return 0;
}

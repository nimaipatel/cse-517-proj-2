#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <linux/perf_event.h>

#include "memlib.h"
#include "string_view.h"
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
        String_View input = String_View_Read_File(traces[i]);
        Trace trace = Trace_Parse(input);
        Trace_Run_Result result = Trace_Run(trace);
        Vec_U64_Stats_Result malloc_stats = Vec_U64_Stats(result.malloc_inst);
        Vec_U64_Stats_Result realloc_stats = Vec_U64_Stats(result.realloc_inst);
        Vec_U64_Stats_Result free_stats = Vec_U64_Stats(result.free_inst);
        Vec_F64_Stats_Result util_stats = Vec_F64_Stats(&result.util_vec);

        printf("%s:\n", traces[i]);
        printf("malloc: %f ± %f\n", malloc_stats.mean,
               malloc_stats.margin_of_error);
        printf("realloc: %f ± %f\n", realloc_stats.mean,
               realloc_stats.margin_of_error);
        printf("free: %f ± %f\n", free_stats.mean, free_stats.margin_of_error);
        printf("util: %f ± %f\n", util_stats.mean, util_stats.margin_of_error);
        printf("\n");
    }

    Heap_Sim_Release();
    return 0;
}


OVERVIEW
========

This project reports performance results for a generic heap allocator by running
it on traces from different programs. You can find the traces in the ./traces
folder, and can register them in the traces[] array inside main.c.

Refer to next sections to learn how to generate traces for your own programs and
run them.

ALLOCATOR CUSTOMIZATIONS
========================

The allocator can be customized using config.h.

In this experiment we explore:
1) Different binning strategies for the segregated free lists.
2) The search limit for free blocks.
3) A "mini-block optimization" that trades of memory utilization for slower
   worst case performance.
4) Different insertion strategies for the lists of free blocks.


``` config.h
#include "mm.h"

// possible values: integer, 0 = first-fit
#define BEST_FIT_SEARCH_LIMIT 0x10

// possible values: TRUE, FALSE
#define MINI_BLOCK_OPTIMIZATION TRUE

// possible values: ADDRESS_ORDERED, FILO
#define FREE_LIST_INSERT_STRATEGY FILO

// define a free table size and then define the binning strategy in
// Size_Get_Bin_Index(...)
#define FREE_TABLE_SIZE 0x10

// The function takes block_size and returns index of the free list bin it
// should be in.
// Possible values: Linear_Binning, Exponential_Binning, Hybrid_Binning,
// Range_Binning, and you can also define your own function
#define Size_Get_Bin_Index Linear_Binning
```

These customizations can be mixed and matched in different combinations. For
this experiment, we use one configuration as a control and then modify other
properties to observe their effect.


GENERATING TRACES FOR YOUR OWN PROGRAMS
=======================================

First add an mtrace call to the start of the main function of the program you want
to trace.

```
int
main()
{
    mtrace();
    // rest of the code as is ...
}
```

Compile and run the program with whatever flags you need.

```
gcc -g -o my_program my_program.c
./my_program
```

After the program finishes, run it as follows:

```
MALLOC_TRACE=mtrace.log ./my_program ...
```

This will generate a trace in mtrace.log file. Convert it the .rep format used
in CMU malloc lab with the following command:

```
mtrace_to_malloclab.py mtrace.log trace.rep
```

EXECUTING TRACES
================

Add .rep trace file path to the traces array in main.c and run the program.
Then compile and run the program.

```
./build.sh release && ./main
```

This will print performance stats for each trace.
This includes the performance mean and margin of error of the performance metric
for malloc, realloc and free, individually and combined.
It also prints average utilization.

The default performance metric is count of hardware instructions for each
malloc, realloc, and free call.  Parameters to Trace_Run() can be used to change
the performance metric. For more details about these argunments, refer to

```
man perf_event_open
```

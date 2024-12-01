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

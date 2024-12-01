#include "mm.h"

// possible values: integer, 0 = first-fit
#define BEST_FIT_SEARCH_LIMIT 0x10
// possible values: TRUE, FALSE
#define MINI_BLOCK_OPTIMIZATION TRUE
// possible values: ADDRESS_ORDERED, FILO
#define FREE_LIST_INSERT_STRATEGY FILO

// define a free table size and then define the binning strategy in
// Size_Get_Bin_Index(...) The function takes block_size and returns index of
// the free list bin it should be in.
#define FREE_TABLE_SIZE 0x10
size_t
Size_Get_Bin_Index(size_t block_size)
{
    size_t MIN_BLOCK_SIZE = 0x2;
    return MIN((block_size - MIN_BLOCK_SIZE) / 2, FREE_TABLE_SIZE - 1);
}
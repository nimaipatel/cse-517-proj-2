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
#define Size_Get_Bin_Index Linear_Binning


// Below are some example binning stratgies set Size_Get_Bin_Index to whatever
// binning strategy you want to use.

size_t
Linear_Binning(size_t block_size)
{
    assert(block_size >= MIN_BLOCK_SIZE);
    return MIN((block_size - MIN_BLOCK_SIZE) / 2, FREE_TABLE_SIZE - 1);
}

size_t
Exponential_Binning(size_t block_size)
{
    assert(block_size >= MIN_BLOCK_SIZE);

    if (block_size == MIN_BLOCK_SIZE) {
        return 0;
    }

    size_t bin = 0;
    size_t limit = 2;

    while (block_size > limit && bin + 1 < FREE_TABLE_SIZE) {
        bin += 1;
        limit *= 2;
    }

    return bin;
}

size_t
Hybrid_Binning(size_t block_size)
{
    assert(block_size >= MIN_BLOCK_SIZE);

    const size_t THRESHOLD_BLOCK_SIZE = MIN_BLOCK_SIZE + 32;

    if (block_size < THRESHOLD_BLOCK_SIZE) {
        return MIN((block_size - MIN_BLOCK_SIZE) / 2, FREE_TABLE_SIZE - 1);
    } else {
        size_t bin = 0;
        size_t limit = THRESHOLD_BLOCK_SIZE;

        while (block_size > limit && bin + 1 < FREE_TABLE_SIZE) {
            bin += 1;
            limit *= 2;
        }

        return bin;
    }
}

size_t
Range_Binning(size_t block_size)
{
    assert(block_size >= MIN_BLOCK_SIZE);

    const size_t BIN_RANGES[] = {
        MIN_BLOCK_SIZE,        // Bin 0: [MIN_BLOCK_SIZE]
        MIN_BLOCK_SIZE + 2,    // Bin 1: [MIN_BLOCK_SIZE + 1, MIN_BLOCK_SIZE + 2]
        MIN_BLOCK_SIZE + 4,    // Bin 2: [MIN_BLOCK_SIZE + 3, MIN_BLOCK_SIZE + 4]
        MIN_BLOCK_SIZE + 8,    // Bin 3: [MIN_BLOCK_SIZE + 5, MIN_BLOCK_SIZE + 8]
        MIN_BLOCK_SIZE + 16,   // Bin 4: [MIN_BLOCK_SIZE + 9, MIN_BLOCK_SIZE + 16]
        MIN_BLOCK_SIZE + 32,   // Bin 5: [MIN_BLOCK_SIZE + 17, MIN_BLOCK_SIZE + 32]
        MIN_BLOCK_SIZE + 64,   // Bin 6: [MIN_BLOCK_SIZE + 33, MIN_BLOCK_SIZE + 64]
        MIN_BLOCK_SIZE + 128   // Bin 7+: [MIN_BLOCK_SIZE + 65, ∞)
    };

    const size_t NUM_BINS = sizeof(BIN_RANGES) / sizeof(BIN_RANGES[0]);

    for (size_t bin = 0; bin < NUM_BINS; ++bin) {
        if (block_size <= BIN_RANGES[bin]) {
            return MIN(bin, FREE_TABLE_SIZE - 1);
        }
    }

    // If no range matches, assign the last bin
    return MIN(NUM_BINS - 1, FREE_TABLE_SIZE - 1);
}
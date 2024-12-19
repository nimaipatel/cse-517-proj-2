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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "heapsim.h"
#include "defines.h"
#include "config.h"

#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
#define dbg_printf(...)
#define dbg_assert(...)
#endif // DEBUG

#define ALIGNMENT (2 * sizeof(Word))
#define WORD_SIZE_BITS (8 * sizeof(Word))

// The smallest free block will at least store a header, footer, two pointers
// and two words each of which are one word long...
static_assert(MIN_BLOCK_SIZE == 0x2, "");

static Word *free_table[FREE_TABLE_SIZE] = { 0 };
static_assert(sizeof(free_table) <= 128, "");

#ifndef BEST_FIT_SEARCH_LIMIT
#error BEST_FIT_SEARCH_LIMIT is not defined...
#endif

#ifdef MINI_BLOCK_OPTIMIZATION
#if MINI_BLOCK_OPTIMIZATION != TRUE && MINI_BLOCK_OPTIMIZATION != FALSE
#error MINI_BLOCK_OPTIMIZATION should be TRUE or FALSE
#endif
#else
#error MINI_BLOCK_OPTIMIZATION is not defined...
#endif

#ifdef FREE_LIST_INSERT_STRATEGY
#if FREE_LIST_INSERT_STRATEGY != ADDRESS_ORDERED && FREE_LIST_INSERT_STRATEGY != FILO
#error FREE_LIST_INSERT_STRATEGY should be ADDRESS_ORDERED or FILO
#endif
#else
#error FREE_LIST_INSERT_STRATEGY is not defined...
#endif

#ifndef FREE_TABLE_SIZE
#error FREE_TABLE_SIZE is not defined...
#endif

static bool Heap_Check(size_t lineno);

// Returns whether the pointer is in the heap.
// May be useful for debugging.
static bool
in_heap(const void *p)
{
    return Heap_Sim_Get_Low() <= p && p <= Heap_Sim_Get_High();
}

// Get the printable string representation of a boolean.
static inline const char *
Bool_Str(const bool b)
{
    return b ? "true" : "false";
}

// Rounds up to the nearest multiple of ALIGNMENT.
static inline size_t
align(const size_t x)
{
    return ALIGNMENT * ((x + ALIGNMENT - 1) / ALIGNMENT);
}

// Given some number of bytes, return smallest amount of words we need to store
// it in our allocation implementation.
// NOTE: takes size in bytes and returns size in words!!!
static inline size_t
Aligned_Word_Size(const size_t size_bytes)
{
#if MINI_BLOCK_OPTIMIZATION == TRUE
    return MAX(align(size_bytes + sizeof(Word)) / sizeof(Word), MIN_BLOCK_SIZE);
#else
    return MAX(align(size_bytes + sizeof(Word)) / sizeof(Word), MIN_BLOCK_SIZE + 2);
#endif // MINI_BLOCK_OPTIMIZATION
}

// Make tag from metadata.
static inline Word
Tag_Pack(const size_t size, const bool alloc, const bool prev_alloc, const bool prev_min)
{
    // TODO: footers only need to store size with with the footer optimization now
    // check the size can fit in the number of bits we have available...
    dbg_assert(size < ((size_t)1 << (WORD_SIZE_BITS - 3)) - 1);

    return (size << 3 | (Word)alloc << 2 | (Word)prev_alloc << 1 | (Word)prev_min);
}

// Get size of block from a tag.
static inline size_t
Tag_Get_Size(const Word word)
{
    const Word size = word >> 3;
    dbg_assert(size % 2 == 0);
    return size;
}

// Get allocation status of block from a tag.
static inline bool
Tag_Get_Alloc(const Word word)
{
    return (word >> 2) & 1;
}

// Get previous block allocation status from a tag.
static inline bool
Tag_Get_Prev_Alloc(const Word word)
{
    return (word >> 1) & 1;
}

// Get if previous block is a minimum sized block.
static inline bool
Tag_Get_Prev_Min(const Word word)
{
    return (word >> 0) & 1;
}

// Get size of block, block is a pointer to start of the block.
static inline size_t
Block_Get_Size(const Word *block)
{
    return Tag_Get_Size(block[0]);
}

// Get allocation status of block, block is a pointer to start of the block.
static inline bool
Block_Get_Alloc(const Word *block)
{
    return Tag_Get_Alloc(block[0]);
}

// Get previous block allocation status from the block.
static inline bool
Block_Get_Prev_Alloc(const Word *block)
{
    return Tag_Get_Prev_Alloc(block[0]);
}

// Check if previous block is minimum sized block.
static inline bool
Block_Get_Prev_Min(const Word *block)
{
    return Tag_Get_Prev_Min(block[0]);
}

// Get free block in the free list, before block.
// NOTE: caller should make sure that block size is not MIN_BLOCK_SIZE before
// calling this function since those blocks don't store a prev pointer.
static inline Word *
Block_Get_Prev_Free(const Word *block)
{
    dbg_assert(Block_Get_Size(block) > MIN_BLOCK_SIZE);

    return (Word *)block[2];
}

// Get free block in the free list, after block.
static inline Word *
Block_Get_Next_Free(const Word *block)
{
    return (Word *)block[1];
}

// Set the prev pointer of the free block.
// NOTE: caller should make sure that block size is not MIN_BLOCK_SIZE before
// calling this function since those blocks don't store a prev pointer.
static inline void
Block_Set_Prev_Free(Word *block, const Word *prev)
{
    dbg_assert(Block_Get_Size(block) > MIN_BLOCK_SIZE);

    block[2] = (Word)prev;
}

// Set the next pointer of the free block.
static inline void
Block_Set_Next_Free(Word *block, const Word *next)
{
    block[1] = (Word)next;
}

// Get the block before the given block in the heap.
// NOTE: caller should make sure previous block has a footer, i.e. is a
// free block before calling this function.
static inline Word *
Block_Get_Prev_Adj(Word *block)
{
    dbg_assert(Block_Get_Prev_Alloc(block) == false);

    const size_t prev_size = Block_Get_Prev_Min(block) ? MIN_BLOCK_SIZE : Tag_Get_Size(block[-1]);
    return block - prev_size;
}

// Get the block right after the given block in the heap.
static inline Word *
Block_Get_Next_Adj(Word *block)
{
    return block + Block_Get_Size(block);
}

// This function unlinks the given block from the free list, it assumes that
// the block exists in the free list, and that its allocation status is set to
// false.
static void
Block_Unlink_Free_List(const Word *block)
{
    const size_t block_size = Block_Get_Size(block);
    const size_t bin_index = Size_Get_Bin_Index(block_size);
    Word **head = &free_table[bin_index];
    dbg_assert(*head != NULL);

    Word *prev = NULL;
    if (block_size == MIN_BLOCK_SIZE)
    {
        // block size is MIN_BLOCK_SIZE, so it doesn't hold prev pointer, we
        // will have to traverse the entire list to get the previous block...
        dbg_assert(bin_index == 0);

        Word *curr = *head;
        while (curr && curr != block)
        {
            prev = curr;
            curr = Block_Get_Next_Free(curr);
        }

        dbg_assert(curr != NULL);
    }
    else
    {
        // block size > MIN_BLOCK_SIZE, so we can get the stored prev
        // pointer...
        prev = Block_Get_Prev_Free(block);
    }

    Word *next = Block_Get_Next_Free(block);

    if (prev)
    {
        Block_Set_Next_Free(prev, next);
    }
    else
    {
        dbg_assert(*head == block);
        *head = next;
    }

    if (next && Block_Get_Size(next) != MIN_BLOCK_SIZE)
    {
        Block_Set_Prev_Free(next, prev);
    }
}

// Adds the provided block to the beginning of the free list.
// NOTE: Normally, we never want to call this directly, only call it
// through Block_Coalesce(...), unless we know that the prev and next
// blocks are not free, for example during initialization of the heap.
static void
Block_Insert_Free_List(Word *block)
{
    // TODO: refactor this into a function...
    const size_t block_size = Block_Get_Size(block);
    const size_t bin_index = Size_Get_Bin_Index(block_size);
    Word **head = &free_table[bin_index];

#if FREE_LIST_INSERT_STRATEGY == ADDRESS_ORDERED
    Word *prev = NULL;
    Word *curr = *head;
    while (curr && curr < block)
    {
        prev = curr;
        curr = Block_Get_Next_Free(curr);
    }

    if (prev)
    {
        Block_Set_Next_Free(prev, block);
    }
    else
    {
        *head = block;
    }

    Block_Set_Next_Free(block, curr);
    if (Block_Get_Size(block) != MIN_BLOCK_SIZE)
    {
        Block_Set_Prev_Free(block, prev);
    }

    if (curr && Block_Get_Size(curr) != MIN_BLOCK_SIZE)
    {
        Block_Set_Prev_Free(curr, block);
    }
#elif FREE_LIST_INSERT_STRATEGY == FILO
    if (Block_Get_Size(block) != MIN_BLOCK_SIZE)
    {
        Block_Set_Prev_Free(block, NULL);
    }
    Block_Set_Next_Free(block, *head);

    if (*head && Block_Get_Size(*head) != MIN_BLOCK_SIZE)
    {
        Block_Set_Prev_Free(*head, block);
    }

    *head = block;
#else
#error unknown FREE_LIST_INSERT_STRATEGY...
#endif // ADDRESS_ORDERED_FREE_LIST
}

// Refreshes next blocks knowledge of previous block's state.
// TODO: can this me merged with Block_Coalesce(...)?
// TODO: can this be eliminated with functions that can set header and footer
// of blocks?
static void
Block_Inform_Next(Word *prev)
{
    Word *next = Block_Get_Next_Adj(prev);
    const size_t size = Block_Get_Size(next);
    const size_t prev_size = Block_Get_Size(prev);
    const bool alloc = Block_Get_Alloc(next);
    const bool prev_alloc = Block_Get_Alloc(prev);
    const bool prev_min = (prev_size == MIN_BLOCK_SIZE);
    const Word tag = Tag_Pack(size, alloc, prev_alloc, prev_min);
    next[0] = tag;
    if (!alloc)
    {
        next[size - 1] = tag;
    }
}

// Coalesce the block that is newly marked as free and add it to the free list.
static Word *
Block_Coalesce(Word *block)
{
    size_t size = Block_Get_Size(block);

    Word *next = Block_Get_Next_Adj(block);
    bool next_alloc = block == next || Block_Get_Alloc(next) == true;
    if (!next_alloc)
    {
        size += Block_Get_Size(next);
        Block_Unlink_Free_List(next);
    }

    bool prev_alloc = Block_Get_Prev_Alloc(block);
    bool prev_min = Block_Get_Prev_Min(block);
    if (!prev_alloc)
    {
        block = Block_Get_Prev_Adj(block);
        prev_alloc = Block_Get_Prev_Alloc(block);
        prev_min = Block_Get_Prev_Min(block);
        size += Block_Get_Size(block);
        Block_Unlink_Free_List(block);
    }

    const Word tag = Tag_Pack(size, false, prev_alloc, prev_min);
    block[0] = tag;
    block[size - 1] = tag;

    Block_Insert_Free_List(block);

    Block_Inform_Next(block);

    return block;
}

// Marks the block as free, coalesces and adds to the free list.
static inline Word *
Block_Free(Word *block, const size_t size, const bool prev_alloc, const bool prev_min)
{
    const Word tag = Tag_Pack(size, false, prev_alloc, prev_min);
    block[0] = tag;
    block[size - 1] = tag;
    return Block_Coalesce(block);
}

// This can be a newly unlinked or already allocated block.
// Function assumes that block_size is the size of the block and allocates
// alloc_size number of words.
// Also spawns new free block if space is available.
static void
Block_Alloc(Word *block, const size_t block_size, const size_t alloc_size)
{
    const bool prev_alloc = Block_Get_Prev_Alloc(block);
    const bool prev_min = Block_Get_Prev_Min(block);

#if MINI_BLOCK_OPTIMIZATION == TRUE
    if (block_size - alloc_size < MIN_BLOCK_SIZE)
    {
#else
    if (block_size - alloc_size < MIN_BLOCK_SIZE + 2)
    {
#endif // MINI_BLOCK_OPTIMIZATION
        const Word tag = Tag_Pack(block_size, true, prev_alloc, prev_min);
        block[0] = tag;
        Block_Inform_Next(block);
    }
    else
    {
        const Word tag = Tag_Pack(alloc_size, true, prev_alloc, prev_min);
        block[0] = tag;
        Word *next = Block_Get_Next_Adj(block);
        Block_Free(next, block_size - alloc_size, true, (alloc_size == MIN_BLOCK_SIZE));
    }
}

// Raise the heap by size number of words and return the new free block it
// created, the block is initialized and coalesced.
static Word *
Heap_Grow(size_t size)
{
    dbg_assert(size % 2 == 0);

    Word *p = Heap_Sim_Sbrk(size * sizeof(Word));
    if (p == NULL)
    {
        return NULL;
    }

    // set new heap end boundary tag...
    Word *heapend = p + size - 1;
    *heapend = Tag_Pack(0, true, false, size == MIN_BLOCK_SIZE);

    // set header and footer of new free block...
    Word *block = p - 1;
    block = Block_Free(block, size, Block_Get_Prev_Alloc(block), Block_Get_Prev_Min(block));

    return block;
}

// Initialize: returns false on error, true on success.
bool
M_Init(void)
{
    // one word each for the special tags at start and end of the heap...
    Word *heap_start = Heap_Sim_Sbrk(sizeof(Word) + sizeof(Word));
    if (heap_start == NULL)
    {
        return false;
    }

    dbg_assert(Heap_Sim_Get_Low() == heap_start);

    // re-initialize the free_list_head to NULL in case M_Init() is called
    // multiple times...
    memset(free_table, 0, sizeof(free_table));

    Word *words = heap_start;

    // special boundary tags at ends of the heap...
    //
    // size is 0, so Block_Get_Next_Adj and Block_Get_Prev_Adj of their
    // neighbours yield themselves...
    //
    // alloc is set to true so they don't get coalesced...
    //
    // prev_alloc and prev_min are DON'T CARE at time of initialization since
    // these bits don't make sense at beginning; when first block is allocated,
    // they will be set to correct values...
    words[0] = Tag_Pack(0, true, true, false);
    words[1] = Tag_Pack(0, true, true, false);

    return true;
}

// malloc
void *
M_malloc(const size_t size)
{
    Heap_Check(__LINE__);

    if (size == 0)
    {
        return NULL;
    }

    const size_t aligned_size = Aligned_Word_Size(size);

    // keeps track of number of blocks searched...
    size_t counter = 0;

    // start with list that stores smallest sized blocks that can at least
    // store this block...
    size_t bin_index = Size_Get_Bin_Index(aligned_size);
    Word *block = free_table[bin_index];

    // find first list that is not empty...
    while (block == NULL && bin_index < FREE_TABLE_SIZE)
    {
        block = free_table[bin_index];
        bin_index += 1;
    }

    // find first fit in the selected free list...
    while (block && Block_Get_Size(block) < aligned_size)
    {
        dbg_assert(Block_Get_Alloc(block) == false);

        counter += 1;
        block = Block_Get_Next_Free(block);
    }

    Word *best_block = block;

    // keep searching for a better fit in the same free list up to a limit...
    while (block && Block_Get_Size(block) != aligned_size && counter < BEST_FIT_SEARCH_LIMIT)
    {
        dbg_assert(Block_Get_Alloc(block) == false);

        counter += 1;

        const size_t curr_size = Block_Get_Size(block);
        const size_t best_size = Block_Get_Size(best_block);

        if (aligned_size <= curr_size && curr_size < best_size)
        {
            best_block = block;
        }

        block = Block_Get_Next_Free(block);
    }

    block = best_block;

    if (!block)
    {
        // couldn't find any free block, need to raise heap...
        block = Heap_Grow(aligned_size);
        if (!block)
        {
            return NULL;
        }
    }

    Block_Unlink_Free_List(block);
    Block_Alloc(block, Block_Get_Size(block), aligned_size);

    return (Word *)block + 1;
}

// free
void
M_free(void *ptr)
{
    Heap_Check(__LINE__);

    if (!ptr)
    {
        return;
    }

    // get the block pointer from the data pointer...
    Word *block = (Word *)ptr - 1;

    // mark block as free and inform next adjacent block...
    Block_Free(block, Block_Get_Size(block), Block_Get_Prev_Alloc(block), Block_Get_Prev_Min(block));
}

// realloc
void *
M_realloc(void *ptr, const size_t size)
{
    Heap_Check(__LINE__);

    // if we are shrinking to 0 bytes, it is essentially just a call to free...
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    // if ptr is NULL then this is essentially just a call to malloc...
    if (!ptr)
    {
        return M_malloc(size);
    }

    const size_t aligned_size = Aligned_Word_Size(size);

    Word *block = (Word *)ptr - 1;
    const size_t old_size = Block_Get_Size(block);

    if (aligned_size == old_size)
    {
        return ptr;
    }

    if (aligned_size < old_size)
    {
        // we are shrinking (or maintaining) block size...
        Block_Alloc(block, Block_Get_Size(block), aligned_size);
        return ptr;
    }

    // we are expanding the block size...
    Word *next = Block_Get_Next_Adj(block);
    const bool next_is_free = !Block_Get_Alloc(next);
    size_t next_size = Block_Get_Size(next);

    if (next_is_free && next_size + old_size >= aligned_size)
    {
        // we found a free block next to us and it has enough free space...
        Block_Unlink_Free_List(next);
        Block_Alloc(block, old_size + next_size, aligned_size);
        return ptr;
    }

    // if nothing works, just do the dumb thing...
    void *new = M_malloc(size);
    if (!new)
    {
        return NULL;
    }

    memcpy(new, ptr, old_size * sizeof(Word));
    M_free(ptr);

    return new;
}

// Returns whether the pointer is aligned.
// May be useful for debugging.
static bool
aligned(const void *p)
{
    size_t ip = (size_t)p;
    return align(ip) == ip;
}

#ifdef DEBUG // Block_Print(...)
// Pretty prints a block, used by other debugging related functions.
static void
Block_Print(Word *block)
{
    if (!block)
    {
        const char *fmt = "%-18s %-18s %-18s %-10s %-10s %-10s\n";
        dbg_printf(fmt, "address", "word", "size", "alloc", "prev_alloc", "prev_min");
        return;
    }

    Word *word = block;
    const bool alloc = Block_Get_Alloc(block);
    const size_t size = Block_Get_Size(block);
    const bool prev_alloc = Block_Get_Prev_Alloc(block);
    const bool prev_min = Block_Get_Prev_Min(block);

    const char *fmt = "0x%016lx 0x%016lx 0x%016lx %-10s %-10s %-10s\n";
    dbg_printf(fmt, word, *word, size, Bool_Str(alloc), Bool_Str(prev_alloc), Bool_Str(prev_min));
}
#endif // Block_Print(...)

// Pretty prints the entire heap.
// I use this function to print the heap in gdb using `call Heap_Print()`.
static void
Heap_Print(void)
{
#ifdef DEBUG // Heap_Print(...)
    Word *words = Heap_Sim_Get_Low();

    dbg_assert(words[0] == Tag_Pack(0, true, true, false));

    dbg_printf("\nHeap start...\n");

    Block_Print(NULL);

    Block_Print(&words[0]);

    Word *iter = &words[1];
    while (iter != Block_Get_Next_Adj(iter))
    {
        Block_Print(iter);

        iter = Block_Get_Next_Adj(iter);
    }

    Block_Print(iter);

    dbg_printf("heap end...\n\n");
#endif // Heap_Print(...)
}

// Pretty prints the free list.
// I use this function to print the free block list in gdb using `call
// Free_List_Print()`.
static void
Free_List_Print(void)
{
#ifdef DEBUG // Free_List_Print(...)
    dbg_printf("\nFree lists start...\n");
    Block_Print(NULL);
    for (size_t i = 0; i < FREE_TABLE_SIZE; i += 1)
    {
        Word *block = free_table[i];

        dbg_printf("list %zu...\n", i);
        while (block)
        {
            Block_Print(block);

            block = Block_Get_Next_Free(block);
        }
    }
    dbg_printf("Free lists end...\n\n");
#endif // Free_List_Print(...)
}

// Heap_Check
bool
Heap_Check(size_t lineno)
{
    bool ret = true;

#ifdef DEBUG_HEAPCHECKER

    size_t n_free = 0;
    for (size_t i = 0; i < FREE_TABLE_SIZE; i += 1)
    {
        Word *prev = NULL;
        Word *block = free_table[i];
        while (block)
        {
            n_free += 1;
            // check that blocks in free list are marked free...
            if (Block_Get_Alloc(block) == true)
            {
                ret = false;
                dbg_printf("line %d: block at %p is in free list "
                           "but marked as allocated\n",
                           lineno, block);
            }

            // check prev and next pointers are consistent...
            if (Block_Get_Size(block) > MIN_BLOCK_SIZE && Block_Get_Prev_Free(block) != prev)
            {
                ret = false;
                dbg_printf("line %d: inconsistent prev pointer for block at %p\n", lineno, block);
            }

            if (Size_Get_Bin_Index(Block_Get_Size(block)) != i)
            {
                ret = false;
                dbg_printf("line %d: %p has size %ld but is in bin %ld\n", lineno, block, Block_Get_Size(block), i);
            }

            prev = block;
            block = Block_Get_Next_Free(block);
        }
    }

    size_t n_free2 = 0;
    Word *prev = NULL;
    Word *block = (Word *)Heap_Sim_Get_Low() + 1;
    while (block != Block_Get_Next_Adj(block))
    {
        if (Block_Get_Alloc(block) == false)
        {
            n_free2 += 1;
            Word *next = Block_Get_Next_Adj(block);

            // check that adjacent blocks are not free...
            if (prev && Block_Get_Alloc(prev) == false)
            {
                ret = false;
                dbg_printf("line %d: block at %p is free "
                           "but one before it at %p is also free\n",
                           lineno, block, prev);
            }

            // check that adjacent blocks are not free...
            if (next != block && Block_Get_Alloc(next) == false)
            {
                ret = false;
                dbg_printf("line %d: block at %p is free "
                           "but one after it at %p is also free\n",
                           lineno, block, next);
            }
        }

        if (prev && Block_Get_Prev_Min(block) != (Block_Get_Size(prev) == MIN_BLOCK_SIZE))
        {
            ret = false;
            dbg_printf("line %d: block %p has prev_min set to %d but size of previous block is %ld\n", lineno, block,
                       Block_Get_Prev_Min(block), Block_Get_Size(prev));
        }

        prev = block;
        block = Block_Get_Next_Adj(block);
    }

    // check last byte of boundary tag is exactly at the end of the heap, this
    // should be enough to prove that all pointers before it are in the heap...
    const void *last_byte = (char *)block + 7;
    if (last_byte != Heap_Sim_Get_High())
    {
        ret = false;
        dbg_printf("line %d: boundary tag is not exactly at the end "
                   "of the heap last byte is at %p but end of heap is at %p\n",
                   lineno, last_byte, Heap_Sim_Get_High());
    }

    // check number of free blocks is consistent from free list and heap
    // iteration...
    if (n_free != n_free2)
    {
        ret = false;
        dbg_printf("line %d: while traversing free list found %ld free blocks "
                   ", but while traversing heap, found %ld free blocks\n",
                   lineno, n_free, n_free2);
    }
#endif // DEBUG_HEAPCHECKER

    return ret;
}

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

    if (block_size == MIN_BLOCK_SIZE)
    {
        return 0;
    }

    size_t bin = 0;
    size_t limit = 2;

    while (block_size > limit && bin + 1 < FREE_TABLE_SIZE)
    {
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

    if (block_size < THRESHOLD_BLOCK_SIZE)
    {
        return MIN((block_size - MIN_BLOCK_SIZE) / 2, FREE_TABLE_SIZE - 1);
    }
    else
    {
        size_t bin = 0;
        size_t limit = THRESHOLD_BLOCK_SIZE;

        while (block_size > limit && bin + 1 < FREE_TABLE_SIZE)
        {
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
        MIN_BLOCK_SIZE, // Bin 0: [MIN_BLOCK_SIZE]
        MIN_BLOCK_SIZE + 2, // Bin 1: [MIN_BLOCK_SIZE + 1, MIN_BLOCK_SIZE + 2]
        MIN_BLOCK_SIZE + 4, // Bin 2: [MIN_BLOCK_SIZE + 3, MIN_BLOCK_SIZE + 4]
        MIN_BLOCK_SIZE + 8, // Bin 3: [MIN_BLOCK_SIZE + 5, MIN_BLOCK_SIZE + 8]
        MIN_BLOCK_SIZE + 16, // Bin 4: [MIN_BLOCK_SIZE + 9, MIN_BLOCK_SIZE + 16]
        MIN_BLOCK_SIZE + 32, // Bin 5: [MIN_BLOCK_SIZE + 17, MIN_BLOCK_SIZE + 32]
        MIN_BLOCK_SIZE + 64, // Bin 6: [MIN_BLOCK_SIZE + 33, MIN_BLOCK_SIZE + 64]
        MIN_BLOCK_SIZE + 128 // Bin 7+: [MIN_BLOCK_SIZE + 65, ∞)
    };

    const size_t NUM_BINS = sizeof(BIN_RANGES) / sizeof(BIN_RANGES[0]);

    for (size_t bin = 0; bin < NUM_BINS; ++bin)
    {
        if (block_size <= BIN_RANGES[bin])
        {
            return MIN(bin, FREE_TABLE_SIZE - 1);
        }
    }

    // If no range matches, assign the last bin
    return MIN(NUM_BINS - 1, FREE_TABLE_SIZE - 1);
}

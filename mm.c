/*
 * mm.c
 *
 * Name 1: Nimai Patel
 * PSU ID 1: nmp5838
 *
 * Name 2: Manay Lodha
 * PSU ID 2: mnl5238
 *
 * This is a malloc implementation that returns double word aligned blocks of
 * memory
 *
 * All blocks store a header that is one word long. The first 62 bits (assuming
 * a word is 64 bits on the system) are used to store the size of the block in
 * words, and the next two bits are used to store the allocation status of the
 * *previous* block and the current block.
 *
 * Free blocks store a footer at their last word, which is identical to the
 * header.
 *
 * Free blocks use the two words right after the header to store pointers to
 * the previous and next free blocks.
 *
 * We use a best fit search for finding free blocks in the free list. It
 * searches at most BEST_FIT_SEARCH_LIMIT number of blocks (unless a block
 * wasn't found in these many searches, in which case it will keep searching
 * till it does find a fit).
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

// If you want to enable your debugging output and heap checker code,
// uncomment the following line. Be sure not to have debugging enabled
// in your final submission.
// #define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

typedef u_int64_t word_t;

#define WORD_SIZE (sizeof(word_t))
#define ALIGNMENT (2 * WORD_SIZE)
#define WORD_SIZE_BITS (8 * WORD_SIZE)

// The smallest free block will at least store a header, footer, two pointers
// and two words each of which are one word long...
#define MIN_BLOCK_SIZE (0x04)

// Decided this by trying different values
// TODO: what could be a better way to decide this?
#define BEST_FIT_SEARCH_LIMIT 0x30

static word_t *free_list_head = NULL;
static word_t *heap_start = NULL;

// Returns whether the pointer is in the heap.
// May be useful for debugging.
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

// Max of two integers.
static inline size_t max_i(const size_t a, const size_t b)
{
    return a > b ? a : b;
}

// Rounds up to the nearest multiple of ALIGNMENT.
static inline size_t align(const size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

// Make tag from metadata.
static inline word_t Tag_Pack(const size_t size, const bool alloc, const bool prev_alloc)
{
    // TODO: footers only need to store size with with the footer optimization now
    // check the size can fit in the number of bits we have available...
    dbg_assert(size < ((size_t)1 << (WORD_SIZE_BITS - 2)) - 1);

    return (size << 2 | (word_t)alloc << 1 | (word_t)prev_alloc);
}

// Get allocation status of block from a tag.
static inline bool Tag_Get_Alloc(const word_t word)
{
    return (word >> 1) & 1;
}

// Get size of block from a tag.
static inline size_t Tag_Get_Size(const word_t word)
{
    const word_t size = word >> 2;
    dbg_assert(size % 2 == 0);
    return size;
}

// Get previous block allocation status from a tag.
static inline bool Tag_Get_Prev_Alloc(const word_t word)
{
    return word & 1;
}

// Get previous block from the block.
static inline bool Block_Get_Prev_Alloc(const word_t *block)
{
    return Tag_Get_Prev_Alloc(block[0]);
}

// Get size of block, `block` is a pointer to start of the block.
static inline size_t Block_Get_Size(const word_t *block)
{
    return Tag_Get_Size(block[0]);
}

// Get the size of the block before the given block in the heap.
static inline size_t Block_Get_Prev_Size(const word_t *block)
{
    // NOTE: caller should make sure previous block has a footer, i.e. is a
    // free block before calling this function.
    dbg_assert(Block_Get_Prev_Alloc(block) == false);

    return Tag_Get_Size(block[-1]);
}

// Get allocation status of block, block is a pointer to start of the block.
static inline bool Block_Get_Alloc(const word_t *block)
{
    return Tag_Get_Alloc(block[0]);
}

// Get free block in the free list, before block`.
static inline word_t *Block_Get_Prev_Free(const word_t *block)
{
    return (word_t *)block[1];
}

// Get free block in the free list, after block.
static inline word_t *Block_Get_Next_Free(const word_t *block)
{
    return (word_t *)block[2];
}

// Set the prev pointer of the free block.
static inline void Block_Set_Prev_Free(word_t *block, const word_t *prev)
{
    block[1] = (word_t)prev;
}

// Set the next pointer of the free block.
static inline void Block_Set_Next_Free(word_t *block, const word_t *next)
{
    block[2] = (word_t)next;
}

// Get the block before the given block in the heap.
static inline word_t *Block_Get_Prev_Adj(word_t *block)
{
    // NOTE: caller should make sure previous block has a footer, i.e. is a
    // free block before calling this function.

    return block - Block_Get_Prev_Size(block);
}

// Get the block right after the given block in the heap.
static inline word_t *Block_Get_Next_Adj(word_t *block)
{
    return block + Block_Get_Size(block);
}

// This function unlinks the given block from the free list, it assumes that
// the block exists in the free list, and that its allocation status is set to
// false.
static void Block_Unlink_Free_List(const word_t *block)
{
    dbg_assert(free_list_head != NULL);

    // TODO: add an assert to check that block exists in the freelist

    word_t *prev = Block_Get_Prev_Free(block);
    word_t *next = Block_Get_Next_Free(block);

    if (prev) {
        Block_Set_Next_Free(prev, next);
    } else {
        dbg_assert(free_list_head == block);
        free_list_head = next;
    }

    if (next) {
        Block_Set_Prev_Free(next, prev);
    }
}

// Adds the provided block to the beginning of the free list.
// NOTE: Normally, we never want to call this directly, only call it
// through Block_Coalesce(...), unless we know that the prev and next
// blocks are not free, for example during initialization of the heap.
static void Block_Prepend_Free_List(void *block)
{
    Block_Set_Prev_Free(block, NULL);
    Block_Set_Next_Free(block, free_list_head);

    if (free_list_head) {
        Block_Set_Prev_Free(free_list_head, block);
    }

    free_list_head = block;
}

// Updates the prev_alloc bit for the block after prev.
// TODO: can this me merged with Block_Coalesce(...)
// TODO: can this be eliminated with functions that can set header and footer
// of blocks?
static word_t *Block_Inform_Next(word_t *prev)
{
    word_t *next = Block_Get_Next_Adj(prev);
    const size_t size = Block_Get_Size(next);
    const bool alloc = Block_Get_Alloc(next);
    const bool prev_alloc = Block_Get_Alloc(prev);
    const word_t tag = Tag_Pack(size, alloc, prev_alloc);
    next[0] = tag;
    if (!alloc) {
        next[size - 1] = tag;
    }
    return next;
}

// Coalesce the block that is newly marked as free and add it to the free list.
static void *Block_Coalesce(word_t *block)
{
    size_t size = Block_Get_Size(block);

    word_t *next = Block_Get_Next_Adj(block);
    bool next_alloc = block == next || Block_Get_Alloc(next) == true;
    if (!next_alloc) {
        size += Block_Get_Size(next);
        Block_Unlink_Free_List(next);
    }

    bool prev_alloc = Block_Get_Prev_Alloc(block);
    if (!prev_alloc) {
        word_t *prev = Block_Get_Prev_Adj(block);
        prev_alloc = Block_Get_Prev_Alloc(prev);
        size += Block_Get_Size(prev);
        Block_Unlink_Free_List(prev);
        block = prev;
    }

    const word_t tag = Tag_Pack(size, false, prev_alloc);
    block[0] = tag;
    block[size - 1] = tag;

    Block_Prepend_Free_List(block);

    return block;
}

// marks the block as free, coalesces and adds to the free list.
static inline word_t *Block_Free(word_t *block, const size_t size, const bool prev_alloc)
{
    const word_t tag = Tag_Pack(size, false, prev_alloc);
    block[0] = tag;
    block[size - 1] = tag;
    Block_Inform_Next(block);

    return Block_Coalesce(block);
}

// This can be a newly unlinked or already allocated block.
// Function assumes that block_size is the size of the block and allocates
// alloc_size number of words.
// Also spawns new free block if space is available.
static word_t *Block_Alloc(word_t *block, const size_t block_size, const size_t alloc_size)
{
    const bool prev_alloc = Block_Get_Prev_Alloc(block);

    if (block_size - alloc_size < MIN_BLOCK_SIZE) {
        const word_t tag = Tag_Pack(block_size, true, prev_alloc);
        block[0] = tag;
        return Block_Inform_Next(block);
    } else {
        const word_t tag = Tag_Pack(alloc_size, true, prev_alloc);
        block[0] = tag;
        word_t *next = Block_Get_Next_Adj(block);
        return Block_Free(next, block_size - alloc_size, true);
    }
}

// Raise the heap by size number of words and return the new free block it
// created, the block is initialized and coalesced.
static word_t *Heap_Grow(size_t size)
{
    dbg_assert(size % 2 == 0);

    word_t *p = mem_sbrk(size * WORD_SIZE);
    if (p == NULL) {
        return NULL;
    }

    // set new heap end boundary tag...
    word_t *heapend = p + size - 1;
    *heapend = Tag_Pack(0, true, false);

    // set header and footer of new free block...
    word_t *block = p - 1;
    block = Block_Free(block, size, Block_Get_Prev_Alloc(block));

    return block;
}

// Initialize: returns false on error, true on success.
bool mm_init(void)
{
    dbg_assert(MIN_BLOCK_SIZE % ALIGNMENT == 0);
    dbg_assert(sizeof(void *) == WORD_SIZE);

    // one word each for the special tags at start and end of the heap...
    heap_start = mem_sbrk(WORD_SIZE + WORD_SIZE);
    if (heap_start == NULL) {
        return false;
    }

    // re-initialize the free_list_head to NULL in case mm_init() is called
    // multiple times...
    free_list_head = NULL;

    word_t *words = heap_start;

    // special boundary tags...
    words[0] = Tag_Pack(0, true, true);
    words[1] = Tag_Pack(0, true, true);

    return true;
}

// malloc
void *malloc(const size_t size)
{
    mm_checkheap(__LINE__);

    if (size == 0) {
        return NULL;
    }

    const size_t aligned_size = max_i(align(size + WORD_SIZE) / WORD_SIZE,
            MIN_BLOCK_SIZE);

    // keeps track of number of blocks searched...
    size_t counter = 0;

    // find first fit...
    word_t *block = free_list_head;

    while (block) {
        dbg_assert(Block_Get_Alloc(block) == false);

        counter += 1;

        const size_t curr_size = Block_Get_Size(block);

        if (curr_size >= aligned_size) {
            break;
        }

        block = Block_Get_Next_Free(block);
    }

    word_t *best_block = block;

    // keep searching for a better fit up to a limit...
    while (block && counter < BEST_FIT_SEARCH_LIMIT) {
        dbg_assert(Block_Get_Alloc(block) == false);

        counter += 1;

        const size_t curr_size = Block_Get_Size(block);
        const size_t best_size = Block_Get_Size(best_block);

        if (aligned_size <= curr_size && curr_size < best_size) {
            best_block = block;
        }

        block = Block_Get_Next_Free(block);
    }

    block = best_block;

    if (!block) {
        // couldn't find any free block, need to raise heap...
        block = Heap_Grow(aligned_size);
        if (!block) {
            return NULL;
        }
    }

    Block_Unlink_Free_List(block);
    Block_Alloc(block, Block_Get_Size(block), aligned_size);

    return (word_t *)block + 1;
}

// free
void free(void *ptr)
{
    mm_checkheap(__LINE__);

    if (!ptr) {
        return;
    }

    // get the block pointer from the data pointer...
    word_t *block = (word_t *)ptr - 1;

    // mark block as free and inform next adjacent block...
    Block_Free(block, Block_Get_Size(block), Block_Get_Prev_Alloc(block));
}

// realloc
void *realloc(void *ptr, const size_t size)
{
    mm_checkheap(__LINE__);

    // if we are shrinking to 0 bytes, it is essentially just a call to free...
    if(size == 0) {
        free(ptr);
        return NULL;
    }

    // if ptr is NULL then this is essentially just a call to malloc...
    if(!ptr) {
        return malloc(size);
    }

    const size_t aligned_size = max_i(align(size + WORD_SIZE) / WORD_SIZE,
            MIN_BLOCK_SIZE);

    word_t *block = (word_t *)ptr - 1;
    const size_t old_size = Block_Get_Size(block);

    if (aligned_size <= old_size) {
        // we are shrinking (or maintaining) block size...
        Block_Alloc(block, Block_Get_Size(block), aligned_size);
        return ptr;
    }

    // we are expanding the block size...
    word_t *next = Block_Get_Next_Adj(block);
    const bool next_is_free = !Block_Get_Alloc(next);
    size_t next_size = Block_Get_Size(next);

    if (next_is_free && next_size + old_size >= aligned_size)  {
        // we found a free block next to us and it has enough free space...
        Block_Unlink_Free_List(next);
        Block_Alloc(block, old_size + next_size, aligned_size);
        return ptr;
    }

    // if nothing works, just do the dumb thing...
    void *newptr = malloc(size);
    if(!newptr) {
        return NULL;
    }

    memcpy(newptr, ptr, old_size * WORD_SIZE);
    free(ptr);

    return newptr;

}

// calloc: This function is not tested by mdriver, and has been implemented for
// you.
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

// Returns whether the pointer is aligned.
// May be useful for debugging.
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

#ifdef DEBUG // Block_Print(...)
// Pretty prints a block, used by.
static void Block_Print(word_t *block)
{
    if (!block) {
        const char *fmt = "  %18s" "  %18s" "  %18s" "  %12s" "  %12s" "\n";
        dbg_printf(fmt, "address", "word", "size", "alloc", "prev alloc");
        return;
    }

    word_t *word = block;
    const bool alloc = Block_Get_Alloc(block);
    const size_t size = Block_Get_Size(block);
    const char *alloc_str = alloc ? "true" : "false";
    const bool prev_alloc = Block_Get_Prev_Alloc(*word);
    const char *prev_alloc_str = prev_alloc ? "true" : "false";

    const char *fmt = "  0x%016lx" "  0x%016lx" "  0x%016lx" "  %12s" "  %12s" "\n";
    dbg_printf(fmt, word, *word, size, alloc_str, prev_alloc_str);
}
#endif // Block_Print(...)

#ifdef DEBUG // Heap_Print(...)
// Pretty prints the entire heap.
// I use this function to print the heap in gdb using
// `call Heap_Print()`.
static void Heap_Print(void)
{
    if (!heap_start) {
        dbg_printf("The heap is uninitialized...\n");
        return;
    }

    word_t *words = heap_start;

    dbg_assert(words[0] == Tag_Pack(0, true, true));

    dbg_printf("\nHeap start...\n");

    Block_Print(NULL);

    Block_Print(&words[0]);

    word_t *iter = &words[1];
    while (iter != Block_Get_Next_Adj(iter)) {
        Block_Print(iter);

        iter = Block_Get_Next_Adj(iter);
    }

    Block_Print(iter);

    dbg_printf("heap end...\n\n");

}
#endif // Heap_Print(...)

#ifdef DEBUG // Free_List_Print(...)
// Pretty prints the free list.
// I use this function to print the free block list in gdb using
// `call Free_List_Print()`.
static void Free_List_Print(void)
{
    word_t *block = free_list_head;

    dbg_printf("\nFree list start...\n");

    Block_Print(NULL);

    while(block) {
        Block_Print(block);

        block = Block_Get_Next_Free(block);
    }

    dbg_printf("Free list end...\n\n");
}
#endif // Free_List_Print(...)

// mm_checkheap
bool mm_checkheap(int lineno)
{

    bool ret = true;

#ifdef DEBUG

    // check that all blocks in free list are free...
    size_t n_free = 0;
    word_t *prev = NULL;
    word_t *block = free_list_head;
    while (block) {
        n_free += 1;
        // check that blocks in free list are marked free...
        if (Block_Get_Alloc(block) == true) {
            ret = false;
            dbg_printf("line %d: block at %p is in free list "
                    "but marked as allocated\n", lineno, block);
        }

        // check prev and next pointers are consistent...
        if (Block_Get_Prev_Free(block) != prev) {
            ret = false;
            dbg_printf("line %d: inconsistent prev pointer for block at %p\n",
                    lineno, block);
        }

        prev = block;
        block = Block_Get_Next_Free(block);
    }

    size_t n_free2 = 0;
    prev = NULL;
    block = (word_t *)heap_start + 1;
    while (block != Block_Get_Next_Adj(block)) {
        if (Block_Get_Alloc(block) == false) {
            n_free2 += 1;
            word_t *next = Block_Get_Next_Adj(block);

            // check that adjacent blocks are not free...
            if (prev && Block_Get_Alloc(prev) == false) {
                ret = false;
                dbg_printf("line %d: block at %p is free "
                        "but one before it at %p is also free\n",
                        lineno, block, prev);
            }

            // check that adjacent blocks are not free...
            if (next != block && Block_Get_Alloc(next) == false) {
                ret = false;
                dbg_printf("line %d: block at %p is free "
                        "but one after it at %p is also free\n",
                        lineno, block, next);
            }
        }

        prev = block;
        block = Block_Get_Next_Adj(block);
    }

    // block should be the end boundary tag...
    word_t boundary_tag = *(word_t *)block;
    if (boundary_tag != Tag_Pack(0, true, false) &&
            boundary_tag != Tag_Pack(0, true, true)) {
        ret = false;
        dbg_printf("line %d: heap traversal boundary tag not matching"
                " boundary tag = %ld\n", lineno, boundary_tag);
    }

    // check last byte of boundary tag is exactly at the end of the heap, this
    // should be enough to prove that all pointers before it are in the heap...
    const void *last_byte = (char *)block + 7;
    if (last_byte != mem_heap_hi()) {
        ret = false;
        dbg_printf("line %d: boundary tag is not exactly at the end "
                "of the heap last byte is at %p but end of heap is at %p\n",
                lineno, last_byte, mem_heap_hi());
    }

    // check start of heap is correct...
    if (heap_start != mem_heap_lo()) {
        ret = false;
        dbg_printf("line %d: heap_start isn't set to actual start of heap "
                "heap_start is %p, but should be %p\n",
                lineno, heap_start, mem_heap_lo());
    }

    // check number of free blocks is consistent from free list and heap
    // iteration...
    if (n_free != n_free2) {
        ret = false;
        dbg_printf("line %d: while traversing free list found %ld free blocks "
                ", but while traversing heap, found %ld free blocks\n",
                lineno, n_free, n_free2);
    }

    // TODO: check that allocated blocks don't overlap...


#endif /* DEBUG */

    return ret;
}

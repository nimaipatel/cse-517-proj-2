/*
 * mm.c
 *
 * Name 1: Nimai Patel
 * PSU ID 1: nmp5838
 *
 * Name 2: Manay Lodha
 * PSU ID 2: mnl5238
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * Also, read malloclab.pdf carefully and in its entirety before beginning.
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

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
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


#define ALIGNMENT 0x10

#define WORD_SIZE 0x8

// The smallest block will at least store a header, footer and two pointers
// each of which are one word long...
#define MIN_BLOCK_SIZE (4 * WORD_SIZE)


static void *free_list_head = NULL;

static void *heap_start = NULL;


/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}


/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(const size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}


static inline u_int64_t Pack_Size_Alloc(const u_int64_t size, const bool alloc)
{
    dbg_assert(size < ((u_int64_t)1 << 63) - 1);

    return (size << 1 | (u_int64_t)alloc);
}


static inline bool Tag_Get_Alloc(const u_int64_t word)
{
    return word & 1;
}


static inline u_int64_t Tag_Get_Size(const u_int64_t word)
{
    return word >> 1;
}


static inline u_int64_t Block_Get_Size(const void *block)
{
    const u_int64_t *words = block;
    const u_int64_t size = Tag_Get_Size(words[0]);
#ifdef DEBUG
    // check block size stored in footer...
    const u_int64_t footer = words[size / WORD_SIZE - 1];
    dbg_assert(size == Tag_Get_Size(footer));
#endif
    return size;
}


static inline bool Block_Get_Alloc(const void *block)
{
    const u_int64_t *words = block;
    const bool alloc = Tag_Get_Alloc(words[0]);
#ifdef DEBUG
    // check alloc status stored in footer...
    const u_int64_t size = Tag_Get_Size(words[0]);
    const u_int64_t footer = words[size / WORD_SIZE - 1];
    dbg_assert(alloc == Tag_Get_Alloc(footer));
#endif
    return alloc;
}


static inline void *Block_Get_Prev_Free(const void *block)
{
    const u_int64_t *words = block;
    return (void *)words[1];
}


static inline void *Block_Get_Next_Free(const void *block)
{
    const u_int64_t *words = block;
    return (void *)words[2];
}


static inline void Block_Set_Next_Free(void *block, const void *next)
{
    u_int64_t *words = block;
    words[2] = (u_int64_t)next;
}


static inline void Block_Set_Prev_Free(void *block, const void *prev)
{
    u_int64_t *words = block;
    words[1] = (u_int64_t)prev;
}


static inline void *Block_Get_Next_Adj(const void *block)
{
    const u_int64_t size = Block_Get_Size(block);
    return (char *)block + size;
}


static inline void *Block_Get_Prev_Adj(const void *block)
{
    const u_int64_t *words = block;
    const u_int64_t prev_footer = words[-1];
    return (char *)block - Tag_Get_Size(prev_footer);
}


static inline void Block_Set_Size_Alloc(void *block, const u_int64_t size, const bool alloc)
{
    dbg_assert(size % (2 * WORD_SIZE) == 0);

    u_int64_t *words = block;
    const u_int64_t size_alloc_word = Pack_Size_Alloc(size, alloc);
    words[0] = size_alloc_word;
    words[size / WORD_SIZE - 1] = size_alloc_word;
}


static void Block_Unlink_Free_List(void *block)
{
    void *prev = Block_Get_Prev_Free(block);
    void *next = Block_Get_Next_Free(block);

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


static void Block_Coalesce(void *block)
{
    // TODO
    assert(false);
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    dbg_printf("Debugging is enabled...\n");
    dbg_assert(MIN_BLOCK_SIZE % ALIGNMENT == 0);

    heap_start = mem_sbrk(WORD_SIZE + MIN_BLOCK_SIZE + WORD_SIZE);
    if (heap_start == NULL) {
        return false;
    }

    u_int64_t *words = heap_start;

    // prologue header, simplifies the case where we call Block_Get_Prev_Adj()
    // on the block right after this...
    words[0] = Pack_Size_Alloc(0, true);

    free_list_head = &words[1];
    Block_Set_Size_Alloc(free_list_head, MIN_BLOCK_SIZE, false);

    // epilogue header, simplifies the case where we call Block_Get_Next_Adj()
    // on the block right before this...
    words[5] = Pack_Size_Alloc(0, true);

    return true;
}


static void Block_Allocate(void *block, const u_int64_t size)
{
    dbg_assert(Block_Get_Alloc(block) == false);

    u_int64_t block_size = Block_Get_Size(block);

    if (block_size - size < MIN_BLOCK_SIZE) {
        // the block doesn't have excess space to split and produce a free
        // block...
        Block_Set_Size_Alloc(block, block_size, true);
        Block_Unlink_Free_List(block);
    } else {
        // the block has excess space, it needs to be split to maximize
        // utilization...
        Block_Set_Size_Alloc(block, size, true);
        Block_Coalesce(block);
    }
}


/*
 * malloc
 */
void *malloc(size_t size)
{
    void *result = NULL;

    if (size == 0) {
        return result;
    }

    size = align(size) + 2 * WORD_SIZE;
    if (size < MIN_BLOCK_SIZE) {
        size = MIN_BLOCK_SIZE;
    }

    void *iter = free_list_head;
    while (iter &&
            // TODO: how to make this check only in debug mode?
            Block_Get_Alloc(iter) == false &&
            Block_Get_Size(iter) < size) {
        iter = Block_Get_Next_Free(iter);
    }

    if (iter) {
        // found a free block of suitable size...
        // TODO
        assert(false);
    } else {
        // couldn't find any free block, need to raise heap...
        // TODO
        assert(false);
    }

    return result;
}

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    return;
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    return NULL;
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
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

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */
#endif /* DEBUG */
    return true;
}

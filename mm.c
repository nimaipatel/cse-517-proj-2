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


typedef struct {
    u_int64_t size;
    u_int64_t alloc;
} Tag;


typedef struct {
    void *prev;
    void *next;
} PointerPair;


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
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}


static inline bool Word_Get_Alloc(u_int64_t word)
{
    return word & 1;
}


static inline u_int64_t Word_Get_Size(u_int64_t word)
{
    return word >> 1;
}


static inline u_int64_t Block_Get_Size(void *block)
{
    u_int64_t *words = block;
#ifdef DEBUG
    // TODO: check block size stored in footer...
#endif
    return Word_Get_Size(words[0]);
}


static inline bool Block_Get_Alloc(void *block)
{
    u_int64_t *words = block;
#ifdef DEBUG
    // TODO: check alloc status stored in footer...
#endif
    return Word_Get_Size(words[0]);
    return Word_Get_Size(words[0]);
}


static inline void *Block_Get_Prev(void *block)
{
    u_int64_t *words = block;
    return (void *)words[1];
}


static inline void *Block_Get_Next(void *block)
{
    u_int64_t *words = block;
    return (void *)words[2];
}


static inline u_int64_t Pack_Size_Alloc(u_int64_t size, bool alloc)
{
    dbg_assert(size < ((u_int64_t)1 << 63));

    return (size << 1 | (u_int64_t)alloc);
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

    u_int64_t *iter = heap_start;

    // prologue header
    *iter = Pack_Size_Alloc(0, true);
    iter += 1;

    free_list_head = iter;

    // size and alloc for first block
    *iter = Pack_Size_Alloc(MIN_BLOCK_SIZE, false);
    iter += 1;

    // pointer to next previous block (NULL)
    *iter = (u_int64_t)NULL;
    iter += 1;

    // pointer to next next block (NULL)
    *iter = (u_int64_t)NULL;
    iter += 1;

    // size and alloc for first block
    *iter = Pack_Size_Alloc(MIN_BLOCK_SIZE, false);
    iter += 1;

    // epilogue header
    *iter = Pack_Size_Alloc(0, true);

    return true;
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
            Block_Get_Alloc(iter) == false &&
            Block_Get_Size(iter) < size) {
        iter = Block_Get_Next(iter);
    }

    if (iter) {
        // found a free block of suitable size...
    } else {
        assert(false);
        // couldn't find any free block, need to raise heap...
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

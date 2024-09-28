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

/* What is the correct alignment? */
#define ALIGNMENT 16


#define INITIAL_HEAP_SIZE 0x100


typedef struct {
    u_int64_t size;
    u_int64_t alloc;
} Tag;


typedef struct {
    void *prev;
    void *next;
} PointerPair;


static void *free_list_head = NULL;


/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    dbg_assert(INITIAL_HEAP_SIZE % ALIGNMENT == 0);
    dbg_assert(sizeof(Tag) == ALIGNMENT);
    dbg_assert(sizeof(PointerPair) == ALIGNMENT);

    if (free_list_head) {
        return false;
    }

    Tag *p = mem_sbrk(INITIAL_HEAP_SIZE);
    if (p == NULL) {
        return false;
    }

    Tag *header = p;
    PointerPair *pair = (PointerPair *)(p + 1);
    Tag *footer = header + (INITIAL_HEAP_SIZE / ALIGNMENT - 1);

    header->size = INITIAL_HEAP_SIZE;
    header->alloc = false;

    pair->next = NULL;
    pair->prev = NULL;

    footer->size = INITIAL_HEAP_SIZE;
    footer->alloc = false;

    free_list_head = p;

    return true;
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    /* IMPLEMENT THIS */
    return NULL;
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
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
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

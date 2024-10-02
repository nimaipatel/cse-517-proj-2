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

// The smallest free block will at least store a header, footer, two pointers
// and two words each of which are one word long...
#define MIN_BLOCK_SIZE (0x04 * WORD_SIZE)


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


static inline u_int64_t max_i(const u_int64_t a, const u_int64_t b)
{
    return a > b ? a : b;
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
    if (size > 0) {
        dbg_assert(words[0] == words[size / WORD_SIZE - 1]);
    }
#endif
    return size;
}


static inline bool Block_Get_Alloc(const void *block)
{
    const u_int64_t *words = block;
    const bool alloc = Tag_Get_Alloc(words[0]);
#ifdef DEBUG
    const u_int64_t size = Tag_Get_Size(words[0]);
    if (size > 0) {
        dbg_assert(words[0] == words[size / WORD_SIZE - 1]);
    }
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


static void Block_Unlink_Free_List(const void *block)
{
    dbg_assert(free_list_head != NULL);

    void *prev = Block_Get_Prev_Free(block);
    void *next = Block_Get_Next_Free(block);

    if (!prev && !next) {
        dbg_assert(free_list_head == block);

        free_list_head = NULL;

    } else if (!prev && next) {
        dbg_assert(free_list_head == block);

        free_list_head = next;
        Block_Set_Prev_Free(free_list_head, NULL);

    } else if (prev && !next) {

        Block_Set_Next_Free(prev, NULL);

    } else {

        Block_Set_Prev_Free(next, prev);
        Block_Set_Next_Free(prev, next);

    }
}


static void Block_Prepend_Free_List(void *block)
{
    if (free_list_head == NULL) {
        Block_Set_Next_Free(block, NULL);
        Block_Set_Prev_Free(block, NULL);
        free_list_head = block;
    } else {
        Block_Set_Prev_Free(block, NULL);
        Block_Set_Next_Free(block, free_list_head);
        Block_Set_Prev_Free(free_list_head, block);
        free_list_head = block;
    }
}


static void *Block_Coalesce(void *block)
{
    u_int64_t size = Block_Get_Size(block);

    void *prev = Block_Get_Prev_Adj(block);
    void *next = Block_Get_Next_Adj(block);

    bool prev_is_free = block != prev && Block_Get_Alloc(prev) == false;
    bool next_is_free = block != next && Block_Get_Alloc(next) == false;

    if (!prev_is_free && next_is_free) {

        size += Block_Get_Size(next);
        Block_Unlink_Free_List(next);
        Block_Set_Size_Alloc(block, size, false);

    } else if (prev_is_free && !next_is_free) {

        size += Block_Get_Size(prev);
        Block_Unlink_Free_List(prev);
        block = prev;
        Block_Set_Size_Alloc(block, size, false);

    } else if (prev_is_free && next_is_free) {

        size += Block_Get_Size(prev) + Block_Get_Size(next);
        Block_Unlink_Free_List(prev);
        Block_Unlink_Free_List(next);
        block = prev;
        Block_Set_Size_Alloc(block, size, false);

    }

    Block_Prepend_Free_List(block);

    return block;
}


static void *Heap_Grow(u_int64_t size)
{
    dbg_assert(size % (2 * WORD_SIZE) == 0);

    void *p = mem_sbrk(size);
    if (p == NULL) {
        return NULL;
    }

    u_int64_t *block = (u_int64_t *)p - 1;

    Block_Set_Size_Alloc(block, size, false);

    u_int64_t *epilogue_header = Block_Get_Next_Adj(block);
    *epilogue_header = Pack_Size_Alloc(0, true);

    block = Block_Coalesce(block);
    
    return block;
}


/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    dbg_assert(MIN_BLOCK_SIZE % ALIGNMENT == 0);
    dbg_assert(sizeof(void *) == WORD_SIZE);

    // one word for padding, one free block of minimum size and special
    // epilogue tag at the end
    heap_start = mem_sbrk(WORD_SIZE + MIN_BLOCK_SIZE + WORD_SIZE);
    if (heap_start == NULL) {
        return false;
    }

    u_int64_t *words = heap_start;

    // padding for alignment...
    words[0] = 0;

    Block_Set_Size_Alloc(&words[1], MIN_BLOCK_SIZE, false);
    free_list_head = NULL;
    Block_Prepend_Free_List(&words[1]);

    words[7] = Pack_Size_Alloc(0, true);

    return true;
}


/*
 * malloc
 */
void *malloc(const size_t size)
{
    mm_checkheap(__LINE__);

    if (size == 0) {
        return NULL;
    }

    const u_int64_t aligned_size = max_i(align(size) + 2 * WORD_SIZE, MIN_BLOCK_SIZE);

    void *block = free_list_head;
    while (block &&
#ifdef DEBUG
            // free list should only have blocks that are marked free, we
            // assert this invariant in debug mode
            (dbg_assert(Block_Get_Alloc(block) == false), true) &&
#endif
            Block_Get_Size(block) < aligned_size) {
        block = Block_Get_Next_Free(block);
    }

    if (!block) {
        // couldn't find any free block, need to raise heap...
        block = Heap_Grow(aligned_size);
        if (!block) {
            return NULL;
        }
    }

    u_int64_t block_size = Block_Get_Size(block);

    if (block_size - aligned_size < MIN_BLOCK_SIZE) {
        // the block doesn't have excess space to split and produce a free
        // block...
        Block_Set_Size_Alloc(block, block_size, true);
        Block_Unlink_Free_List(block);
    } else {
        // the block has excess space, it needs to be split to maximize
        // utilization...
        Block_Set_Size_Alloc(block, aligned_size, true);
        Block_Unlink_Free_List(block);

        void *next = Block_Get_Next_Adj(block);
        Block_Set_Size_Alloc(next, block_size - aligned_size, false);
        Block_Coalesce(next);
    }

    return (char *)block + WORD_SIZE;
}

/*
 * free
 */
void free(void* ptr)
{
    mm_checkheap(__LINE__);

    if (!ptr) {
        return;
    }

    void *block = (char *)ptr - WORD_SIZE;

    const u_int64_t size = Block_Get_Size(block);
    Block_Set_Size_Alloc(block, size, false);
    Block_Coalesce(block);
}

/*
 * realloc
 */
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

    const u_int64_t aligned_size = max_i((align(size) + 2 * WORD_SIZE), MIN_BLOCK_SIZE);

    void *block = (u_int64_t *)ptr - 1;
    const u_int64_t old_size = Block_Get_Size(block);

    // we are shrinking (or maintaing) block size...
    if (aligned_size <= old_size) { 
        // ...but there is NOT enough space to spawn a new free block...
        if (old_size - aligned_size < MIN_BLOCK_SIZE) {
            return ptr;
        } else {
            // there is enough space to spawn a new free block...
            // NOTE: adding this case did not increase peformance for some
            // reason
            Block_Set_Size_Alloc(block, aligned_size, true);
            void *next = Block_Get_Next_Adj(block);
            Block_Set_Size_Alloc(next, old_size - aligned_size, false);
            Block_Coalesce(next);
            return ptr;

        }

    } else {
        // we are expanding the block size...

        void *next = Block_Get_Next_Adj(block);
        const bool next_is_free = !Block_Get_Alloc(next);
        const u_int64_t next_size = Block_Get_Size(next);

        // we found a free block next to us and it has enough free space...
        if (next_is_free && next_size + old_size >= size)  {

            Block_Unlink_Free_List(next);

            // the free block will be entirely used...
            if (next_size + old_size - aligned_size <= MIN_BLOCK_SIZE) {
                Block_Set_Size_Alloc(block, next_size + old_size, true);
                return ptr;
            } else {

                // the free block next to us has more space then we need for
                // expanding, it will spawn a new free block...
                Block_Set_Size_Alloc(block, aligned_size, true);

                next = Block_Get_Next_Adj(block);
                Block_Set_Size_Alloc(next, next_size + old_size - aligned_size, false);
                Block_Coalesce(next);

                return ptr;
            }

        } else {

            void *newptr = malloc(size);
            if(!newptr) {
                return NULL;
            }

            memcpy(newptr, ptr, old_size);
            free(ptr);

            return newptr;
        }
    }

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


#ifdef DEBUG // Heap_Print(void)
// I use this function to print the heap in gdb using
// `call Heap_Print()`
static void Heap_Print(void)
{
    if (!heap_start) {
        dbg_printf("The heap is uninitialized...\n");
        return;
    }

    u_int64_t *words = heap_start;
    
    dbg_assert(words[0] == 0);

    dbg_printf("\nHeap start...\n");

    dbg_printf("%p\t0x%016lx\tPadding for alignment\n", words, words[0]);

    u_int64_t *iter = &words[1];
    do {
        const u_int64_t size = Block_Get_Size(iter);
        const bool alloc = Block_Get_Alloc(iter);

        dbg_printf("%p\t0x%016lx\tsize = 0x%16lx\talloc = %d\n", iter, *iter, size, alloc);

        iter = Block_Get_Next_Adj(iter);
    } while (iter != Block_Get_Next_Adj(iter));

    const u_int64_t size = Block_Get_Size(iter);
    const bool alloc = Block_Get_Alloc(iter);

    dbg_printf("%p\t0x%016lx\tsize = 0x%16lx\talloc = %d\tEpilogue...\n", iter, *iter, size, alloc);

    dbg_printf("heap end...\n\n");

}
#endif // Heap_Print(void)


#ifdef DEBUG // Free_List_Print(void)
// I use this function to print the free block list in gdb using 
// `call Free_List_Print()`
static void Free_List_Print(void)
{
    u_int64_t *block = free_list_head;

    dbg_printf("\nFree list start...\n");

    while(block) {
        const u_int64_t size = Block_Get_Size(block);
        const bool alloc = Block_Get_Alloc(block);

        dbg_printf("%p\t0x%016lx\tsize = 0x%lx\talloc = %d\n", block, *block, size, alloc);

        block = Block_Get_Next_Free(block);
    }

    dbg_printf("Free list end...\n\n");
}
#endif // Free_List_Print(void)


/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{

#ifdef DEBUG

    // check that all blocks in free list are free...
    void *block = free_list_head;
    while (block) {
        dbg_assert(Block_Get_Alloc(block) == false);

        block = Block_Get_Next_Free(block);
    }

    // TODO: check  adjacent free blocks are always coalesced...
    // TODO: check that every free block is in the free list...
    // TODO: check pointers in free blocks point to other valid free blocks...
    // TODO: check that allocated blocks don't overlap...
    // TODO: all pointers are in the heap...


#endif /* DEBUG */

    return true;
}

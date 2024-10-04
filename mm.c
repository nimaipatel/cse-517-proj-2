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

typedef u_int64_t word_t;

#define WORD_SIZE (sizeof(word_t))
#define ALIGNMENT (2 * WORD_SIZE)
#define WORD_SIZE_BITS (8 * WORD_SIZE)

// The smallest free block will at least store a header, footer, two pointers
// and two words each of which are one word long...
#define MIN_BLOCK_SIZE (0x04 * WORD_SIZE)

// Decided this by trying different values
// TODO: what could be a better way to decide this?
#define BEST_FIT_SEARCH_LIMIT 0x30

// this is the result of Pack_Size_Alloc(0, true)
// size = 0, and last bit is set to 1 (true) for allocation status
#define BOUNDARY_TAG 0x0000000000000001

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


static inline word_t max_i(const word_t a, const word_t b)
{
    return a > b ? a : b;
}


/* rounds up to the nearest multiple of ALIGNMENT */
static inline size_t align(const size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}


/* packs a size and allocation status bit into a single word which will be used
 * as a block tag i.e. header or footer*/
static inline word_t Pack_Size_Alloc(const word_t size, const bool alloc)
{
    dbg_assert(size < ((word_t)1 << (WORD_SIZE_BITS - 1)) - 1);

    return (size << 1 | (word_t)alloc);
}



/* get allocation status of block from a tag */
static inline bool Tag_Get_Alloc(const word_t word)
{
    return word & 1;
}


/* get size of block from a tag */
static inline word_t Tag_Get_Size(const word_t word)
{
    const word_t size = word >> 1;
    dbg_assert(size % (2 * WORD_SIZE) == 0);
    return size;
}


/* get size of block, `block` is a pointer to start of the block */
static inline word_t Block_Get_Size(const void *block)
{
    const word_t *words = block;
    const word_t size = Tag_Get_Size(words[0]);
#ifdef DEBUG
    // we need to check if size is non-zero, since this calculation won't work
    // on special boundary tag
    if (size > 0) {
        dbg_assert(words[0] == words[size / WORD_SIZE - 1]);
    }
#endif
    return size;
}


/* get allocation status of block, `block` is a pointer to start of the block */
static inline bool Block_Get_Alloc(const void *block)
{
    const word_t *words = block;
    const bool alloc = Tag_Get_Alloc(words[0]);
#ifdef DEBUG
    const word_t size = Tag_Get_Size(words[0]);
    // we need to check if size is non-zero, since this calculation won't work
    // on special boundary tag
    if (size > 0) {
        dbg_assert(words[0] == words[size / WORD_SIZE - 1]);
    }
#endif
    return alloc;
}


/* get free block in the free list, before `block` */
static inline void *Block_Get_Prev_Free(const void *block)
{
    const word_t *words = block;
    return (void *)words[1];
}


/* get free block in the free list, after `block` */
static inline void *Block_Get_Next_Free(const void *block)
{
    const word_t *words = block;
    return (void *)words[2];
}


/* set the prev pointer of the free block */
static inline void Block_Set_Prev_Free(void *block, const void *prev)
{
    word_t *words = block;
    words[1] = (word_t)prev;
}


/* set the next pointer of the free block */
static inline void Block_Set_Next_Free(void *block, const void *next)
{
    word_t *words = block;
    words[2] = (word_t)next;
}


/* get the block before the given block in the heap */
static inline void *Block_Get_Prev_Adj(const void *block)
{
    const word_t *words = block;
    const word_t prev_footer = words[-1];
    return (char *)block - Tag_Get_Size(prev_footer);
}


/* get the block right after the given block in the heap */
static inline void *Block_Get_Next_Adj(const void *block)
{
    const word_t size = Block_Get_Size(block);
    return (char *)block + size;
}


/* set the size and allocation status of given block, this edits both the
 * header and footer of the block */
static inline void Block_Set_Size_Alloc(void *block, const word_t size, const bool alloc)
{
    dbg_assert(size % (2 * WORD_SIZE) == 0);

    word_t *words = block;
    const word_t size_alloc_word = Pack_Size_Alloc(size, alloc);
    words[0] = size_alloc_word;
    words[size / WORD_SIZE - 1] = size_alloc_word;
}


/* this function unlinks the given block from the free list, it assumes that
 * the block exists in the free list, and that its allocation status is set to
 * false */
static void Block_Unlink_Free_List(const void *block)
{
    dbg_assert(free_list_head != NULL);

    // TODO: add an assert to check that block exists in the freelist

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


/* adds the provided block to the beginning of the free list.
 * NOTE: Normally, we never want to call this directly, only call it through
 * Block_Coalesce(...), unless we know that the prev and next blocks are not
 * free, for example during initialization of the heap */
static void Block_Prepend_Free_List(void *block)
{

    Block_Set_Prev_Free(block, NULL);
    Block_Set_Next_Free(block, free_list_head);

    if (free_list_head) {
        Block_Set_Prev_Free(free_list_head, block);
    }

    free_list_head = block;
}


static void *Block_Coalesce(void *block)
{
    word_t size = Block_Get_Size(block);

    void *prev = Block_Get_Prev_Adj(block);
    void *next = Block_Get_Next_Adj(block);

    bool prev_is_free = block != prev && Block_Get_Alloc(prev) == false;
    bool next_is_free = block != next && Block_Get_Alloc(next) == false;

    if (prev_is_free) {
        block = prev;
        size += Block_Get_Size(prev);
        Block_Unlink_Free_List(prev);
    }

    if (next_is_free) {
        size += Block_Get_Size(next);
        Block_Unlink_Free_List(next);
    }

    Block_Set_Size_Alloc(block, size, false);

    Block_Prepend_Free_List(block);

    return block;
}


static void *Heap_Grow(word_t size)
{
    dbg_assert(size % (2 * WORD_SIZE) == 0);

    void *p = mem_sbrk(size);
    if (p == NULL) {
        return NULL;
    }

    word_t *block = (word_t *)p - 1;

    Block_Set_Size_Alloc(block, size, false);

    word_t *head_end = Block_Get_Next_Adj(block);
    *head_end = BOUNDARY_TAG;

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

    // one word each for the special tags at start and end of the heap...
    heap_start = mem_sbrk(WORD_SIZE + WORD_SIZE);
    if (heap_start == NULL) {
        return false;
    }

    // re-initialize the free_list_head to NULL in case mm_init() is called
    // multiple times
    free_list_head = NULL;

    word_t *words = heap_start;

    // special tags at start and end of the heap...
    words[0] = BOUNDARY_TAG;
    words[1] = BOUNDARY_TAG;

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

    const word_t aligned_size = max_i(align(size) + 2 * WORD_SIZE, MIN_BLOCK_SIZE);

    // keeps track of number of blocks searched...
    size_t counter = 0;

    // find first fit...
    void *block = free_list_head;

    while (block) {
        dbg_assert(Block_Get_Alloc(block) == false);

        counter += 1;

        const word_t curr_size = Block_Get_Size(block);

        if (curr_size >= aligned_size) {
            break;
        }

        block = Block_Get_Next_Free(block);
    }

    void *best_block = block;

    // keep searching for a better fit up to a limit...
    while (block && counter < BEST_FIT_SEARCH_LIMIT) {
        dbg_assert(Block_Get_Alloc(block) == false);

        counter += 1;

        const word_t curr_size = Block_Get_Size(block);
        const word_t best_size = Block_Get_Size(best_block);

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

    word_t block_size = Block_Get_Size(block);

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

    return (word_t *)block + 1;
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

    void *block = (word_t *)ptr - 1;

    const word_t size = Block_Get_Size(block);
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

    const word_t aligned_size = max_i((align(size) + 2 * WORD_SIZE), MIN_BLOCK_SIZE);

    void *block = (word_t *)ptr - 1;
    const word_t old_size = Block_Get_Size(block);

    if (aligned_size <= old_size) {
        // we are shrinking (or maintaining) block size...

        if (old_size - aligned_size < MIN_BLOCK_SIZE) {
            // ...but there is NOT enough space to spawn a new free block...
            return ptr;

        } else {
            // there is enough space to spawn a new free block...
            // NOTE: adding this case did not increase performance for some
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
        const word_t next_size = Block_Get_Size(next);

        if (next_is_free && next_size + old_size >= aligned_size)  {
            // we found a free block next to us and it has enough free space...

            Block_Unlink_Free_List(next);

            if (next_size + old_size - aligned_size < MIN_BLOCK_SIZE) {
                // the free block will be entirely used...

                Block_Set_Size_Alloc(block, next_size + old_size, true);
                return ptr;

            } else {
                // the free block next to us has more space than we need for
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


#ifdef DEBUG // Block_Print(...)
static void Block_Print(void *block)
{
    if (!block) {
        const char *fmt = "  %18s" "  %18s" "  %18s" "  %18s" "\n";
        dbg_printf(fmt, "address", "word", "size", "allocation");
        return;
    }

    word_t *word = block;
    const bool alloc = Block_Get_Alloc(block);
    const word_t size = Block_Get_Size(block);
    const char *alloc_str = alloc ? "true" : "false";

    const char *fmt = "  0x%016lx" "  0x%016lx" "  0x%016lx" "  %18s" "\n";
    dbg_printf(fmt, word, *word, size, alloc_str);
}
#endif // Block_Print(...)


#ifdef DEBUG // Heap_Print(...)
// I use this function to print the heap in gdb using
// `call Heap_Print()`
static void Heap_Print(void)
{
    if (!heap_start) {
        dbg_printf("The heap is uninitialized...\n");
        return;
    }

    word_t *words = heap_start;

    dbg_assert(words[0] == BOUNDARY_TAG);

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
// I use this function to print the free block list in gdb using
// `call Free_List_Print()`
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


/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{

    bool ret = true;

#ifdef DEBUG

    // check that all blocks in free list are free...
    size_t n_free = 0;
    void *prev = NULL;
    void *block = free_list_head;
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
    block = (word_t *)heap_start + 1;
    while (block != Block_Get_Next_Adj(block)) {
        if (Block_Get_Alloc(block) == false) {
            n_free2 += 1;
            void *prev = Block_Get_Prev_Adj(block);
            void *next = Block_Get_Prev_Adj(block);

            // check that adjacent blocks are not free...
            if (prev != block && Block_Get_Alloc(prev) == false) {
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

        block = Block_Get_Next_Adj(block);
    }

    // block should be the end boundary tag...
    if (*(word_t *)block != BOUNDARY_TAG) {
        ret = false;
        dbg_printf("line %d: heap traversal stopped before reaching the"
                " boundary tag\n", lineno);
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

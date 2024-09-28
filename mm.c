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
    u_int64_t a;
    u_int64_t b;
} DoubleWord;


typedef struct {
    u_int64_t size;
    u_int64_t alloc;
} Tag;


typedef struct {
    void *prev;
    void *next;
} PointerPair;


static void *free_list_head = NULL;


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


static inline u_int64_t Block_Get_Size(void *ptr) {
    Tag *t = ptr;
#ifdef DEBUG
    u_int64_t size = t->size;
    dbg_assert(size == (t + size / 16 - 1)->size);
#endif
    return t->size;
}


static inline u_int64_t Block_Get_Alloc(void *ptr) {
    Tag *t = ptr;
#ifdef DEBUG
    u_int64_t size = t->size;
    dbg_assert(t->alloc == (t + size / 16 - 1)->alloc);
#endif
    return t->alloc;
}


static inline void * Block_Get_Next(void *ptr) {
    Tag *t = ptr;
    PointerPair *pp = (PointerPair *)(t + 1);
    return pp->next;
}


static inline void * Block_Get_Prev(void *ptr) {
    Tag *t = ptr;
    PointerPair *pp = (PointerPair *)(t + 1);
    return pp->prev;
}


static inline void Block_Set_Size(void *ptr, u_int64_t size) {
    dbg_assert(size % 16 == 0);

    Tag *t = ptr;
    t->size = size;
    (t + size / 16 - 1)->size = size;
}


static inline void Block_Set_Alloc(void *ptr, u_int64_t alloc, u_int64_t size) {
    dbg_assert(size % 16 == 0);

    Tag *t = ptr;
    t->alloc = alloc;
    (t + size / 16 - 1)->alloc = alloc;
}


static inline void Block_Set_Next(void *ptr, void *next) {
    Tag *tag = ptr;
    PointerPair *pp = (PointerPair *)(tag + 1);
    pp->next = next;
}


static inline void Block_Set_Prev(void *ptr, void *prev) {
    Tag *tag = ptr;
    PointerPair *pp = (PointerPair *)(tag + 1);
    pp->prev = prev;
}

static inline void * Block_Get_Data_Ptr(void *block) {
    Tag *t = block;
    Tag *data = t + 1;
    return (void *)data;
}


/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    dbg_printf("Debugging is enabled...\n");
    dbg_assert(INITIAL_HEAP_SIZE % ALIGNMENT == 0);
    dbg_assert(sizeof(DoubleWord) == ALIGNMENT);
    dbg_assert(sizeof(Tag) == sizeof(DoubleWord));
    dbg_assert(sizeof(PointerPair) == sizeof(DoubleWord));

    void *p = mem_sbrk(INITIAL_HEAP_SIZE);
    if (p == NULL) {
        return false;
    }

    Block_Set_Size(p, INITIAL_HEAP_SIZE);
    Block_Set_Alloc(p, false, INITIAL_HEAP_SIZE);

    Block_Set_Next(p, NULL);
    Block_Set_Prev(p, NULL);

    free_list_head = p;

    return true;
}


/*
 * malloc
 */
void* malloc(size_t size)
{
    void *result = NULL;

    if (size == 0) {
        return result;
    }

    size = align(size + 2 * sizeof(Tag));

    void *curr = free_list_head;
    while (curr && Block_Get_Size(curr) < size) {
        curr = Block_Get_Next(curr);
    }


    if (curr) {
        void *prev = Block_Get_Prev(curr);
        void *next = Block_Get_Next(curr);

        void *alloc_block = curr;
        result = Block_Get_Data_Ptr(curr);

        u_int64_t free_size = Block_Get_Size(curr);

        Block_Set_Size(alloc_block, size);
        Block_Set_Alloc(alloc_block, true, size);

        if (free_size > size) {
            void *free_block = ((char *)alloc_block) + size;
            Block_Set_Size(free_block, free_size - size);
            Block_Set_Alloc(free_block, false, free_size - size);
            Block_Set_Prev(free_block, prev);
            Block_Set_Next(free_block, next);

            if (prev) {
                Block_Set_Next(prev, free_block);
            } else {
                dbg_assert(free_list_head == curr);
                free_list_head = free_block;
            }

            if (next) {
                Block_Set_Prev(next, free_block);
            }

        } else {
            if (prev) {
                Block_Set_Next(prev, next);
            } else {
                dbg_assert(free_list_head == curr);
                free_list_head = next;
            }

            if (next) {
                Block_Set_Prev(next, prev);
            }
        }

    } else {
        void *p = mem_sbrk(size);

        Tag *t = p;
        Tag *prev_footer = t - 1;
        if (in_heap(prev_footer) && prev_footer->alloc == false) {
            u_int64_t prev_block_size = prev_footer->size;
            Tag *header = prev_footer - prev_block_size / 16 + 1;

            void *prev = Block_Get_Prev(header);
            void *next = Block_Get_Next(header);

            size += header->size;
            Block_Set_Size(header, size);
            Block_Set_Alloc(header, true, size);

            Block_Set_Prev(header, prev);
            Block_Set_Next(header, next);

            p = header;
        } else {
            Block_Set_Prev(p, NULL);
            Block_Set_Next(p, free_list_head);

            Block_Set_Prev(free_list_head, p);

            free_list_head = p;
        }

        result = Block_Get_Data_Ptr(p);
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

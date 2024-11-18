#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "memlib.h"
#include "defines.h"

static U8 *heap;
static U8 *mem_brk;
static U8 *mem_max_addr;

// mem_init - initialize the memory system model
void
mem_init(void)
{
    U8 *addr = mmap(NULL, MAX_HEAP_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1,
                   0);
    if (addr == MAP_FAILED) {
        fprintf(stderr, "FAILURE.  mmap couldn't allocate space for heap\n");
        exit(1);
    }
    heap = addr;
    mem_max_addr = addr + MAX_HEAP_SIZE;
    mem_reset_brk();
}

// mem_deinit - free the storage used by the memory system model
void
mem_deinit(void)
{
    if (munmap(heap, MAX_HEAP_SIZE) != 0) {
        fprintf(stderr, "FAILURE.  munmap couldn't deallocate heap space\n");
        exit(1);
    }
}

// mem_reset_brk - reset the simulated brk pointer to make an empty heap
void
mem_reset_brk(void)
{
    mem_brk = heap;
}

// mem_sbrk - simple model of the sbrk function. Extends the heap by incr bytes
// and returns the start address of the new area. In this model, the heap cannot
// be shrunk.
void *
mem_sbrk(intptr_t incr)
{
    U8 *old_brk = mem_brk;

    bool ok = true;
    if (incr < 0) {
        ok = false;
        fprintf(
            stderr,
            "ERROR: mem_sbrk failed.  Attempt to expand heap by negative value %ld\n",
            (long)incr);
    } else if (mem_brk + incr > mem_max_addr) {
        ok = false;
        long alloc = mem_brk - heap + incr;
        fprintf(
            stderr,
            "ERROR: mem_sbrk failed. Ran out of memory.  Would require heap size of %zd (0x%zx) bytes\n",
            alloc, alloc);
    }
    if (ok) {
        mem_brk += incr;
        return (void *)old_brk;
    } else {
        errno = ENOMEM;
        return (void *)-1;
    }
}

// mem_heap_lo - return address of the first heap byte
void *
mem_heap_lo(void)
{
    return (void *)heap;
}

// mem_heap_hi - return address of last heap byte
void *
mem_heap_hi(void)
{
    return (void *)(mem_brk - 1);
}

// mem_heapsize() - returns the heap size in bytes
size_t
mem_heapsize(void)
{
    return (size_t)(mem_brk - heap);
}

// mem_pagesize() - returns the page size of the system
size_t
mem_pagesize(void)
{
    return (size_t)getpagesize();
}
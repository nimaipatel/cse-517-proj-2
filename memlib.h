#ifndef _MEMLIB_H
#define _MEMLIB_H

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_HEAP_SIZE (1ull * (1ull << 40)) /* 1 TB */

void mem_init(void);
void mem_deinit(void);
void *mem_sbrk(intptr_t incr);
void mem_reset_brk(void);
void *mem_heap_lo(void);
void *mem_heap_hi(void);
size_t mem_heapsize(void);
size_t mem_pagesize(void);

#endif // _MEMLIB_H

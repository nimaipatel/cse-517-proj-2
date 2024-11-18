#ifndef _MEMLIB_H
#define _MEMLIB_H

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_HEAP_SIZE (1ull * (1ull << 40)) /* 1 TB */

void Heap_Sim_Init(void);
void Heap_Sim_Release(void);
void *Heap_Sim_Sbrk(intptr_t incr);
void Heap_Sim_Brk(void);
void *Heap_Sim_Get_Low(void);
void *Heap_Sim_Get_High(void);
size_t Heap_Sim_Get_Heap_Size(void);
size_t Heap_Sim_Get_Page_Size(void);

#endif // _MEMLIB_H

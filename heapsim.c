/*
 * Copyright (C) 2024 Patel, Nimai <nimai.m.patel@gmail.com>
 * Author: Patel, Nimai <nimai.m.patel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

#include "heapsim.h"
#include "defines.h"

static U8 *heap;
static U8 *mem_brk;
static U8 *mem_max_addr;

void
Heap_Sim_Init(void)
{
    U8 *addr = mmap(NULL, MAX_HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (addr == MAP_FAILED)
    {
        fprintf(stderr, "mmap failed\n");
        exit(1);
    }
    heap = addr;
    mem_max_addr = addr + MAX_HEAP_SIZE;
    Heap_Sim_Brk();
}

void
Heap_Sim_Release(void)
{
    if (munmap(heap, MAX_HEAP_SIZE) != 0)
    {
        fprintf(stderr, "munmap failed\n");
        exit(1);
    }
}

void
Heap_Sim_Brk(void)
{
    mem_brk = heap;
}

void *
Heap_Sim_Sbrk(intptr_t incr)
{
    U8 *old_brk = mem_brk;

    bool ok = true;
    if (incr < 0)
    {
        ok = false;
        fprintf(stderr, "ERROR: Heap_Sim_Sbrk failed.  Attempt to expand heap by negative value %ld\n", (long)incr);
    }
    else if (mem_brk + incr > mem_max_addr)
    {
        ok = false;
        long alloc = mem_brk - heap + incr;
        fprintf(stderr,
                "ERROR: Heap_Sim_Sbrk failed. Ran out of memory.  Would require heap size of %zd (0x%zx) bytes\n",
                alloc, alloc);
    }
    if (ok)
    {
        mem_brk += incr;
        return (void *)old_brk;
    }
    else
    {
        errno = ENOMEM;
        return (void *)-1;
    }
}

void *
Heap_Sim_Get_Low(void)
{
    return (void *)heap;
}

void *
Heap_Sim_Get_High(void)
{
    return (void *)(mem_brk - 1);
}

size_t
Heap_Sim_Get_Heap_Size(void)
{
    return (size_t)(mem_brk - heap);
}

size_t
Heap_Sim_Get_Page_Size(void)
{
    return (size_t)getpagesize();
}

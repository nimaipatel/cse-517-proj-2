#include "mm.h"
#include "memlib.h"
#include <stdio.h>

int
main(void)
{
    mem_init();

    mm_init();

    int *some_ptr;
    some_ptr = mm_malloc(sizeof(*some_ptr));
    printf("pointer some_ptr = 0x%lx\n", (unsigned long)(some_ptr));
    mm_free(some_ptr);
    some_ptr = mm_malloc(sizeof(*some_ptr));
    printf("pointer some_ptr = 0x%lx\n", (unsigned long)(some_ptr));
    some_ptr = mm_malloc(sizeof(*some_ptr));
    printf("pointer some_ptr = 0x%lx\n", (unsigned long)(some_ptr));
    some_ptr = mm_malloc(sizeof(*some_ptr));
    printf("pointer some_ptr = 0x%lx\n", (unsigned long)(some_ptr));
    some_ptr = mm_malloc(sizeof(*some_ptr));
    printf("pointer some_ptr = 0x%lx\n", (unsigned long)(some_ptr));

    mem_deinit();
    return 0;
}

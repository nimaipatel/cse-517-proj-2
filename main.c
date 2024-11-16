#include "mm.h"
#include "memlib.h"
#include <stdio.h>

int
main(void)
{
    mem_init();

    mm_init();

    int *nice;
    nice = mm_malloc(sizeof(*nice));
    printf("pointer nice = 0x%lx\n", (unsigned long)(nice));
    mm_free(nice);
    nice = mm_malloc(sizeof(*nice));
    printf("pointer nice = 0x%lx\n", (unsigned long)(nice));
    nice = mm_malloc(sizeof(*nice));
    printf("pointer nice = 0x%lx\n", (unsigned long)(nice));
    nice = mm_malloc(sizeof(*nice));
    printf("pointer nice = 0x%lx\n", (unsigned long)(nice));
    nice = mm_malloc(sizeof(*nice));
    printf("pointer nice = 0x%lx\n", (unsigned long)(nice));

    mem_deinit();
    return 0;
}


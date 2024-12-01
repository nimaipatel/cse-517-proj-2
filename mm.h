#ifndef _MM_H
#define _MM_H

#include <stdbool.h>
#include <stdlib.h>

void *M_malloc(size_t size);
void M_free(void *ptr);
void *M_realloc(void *ptr, size_t size);
void *M_calloc(size_t nmemb, size_t size);
bool M_Init(void);

#define MIN_BLOCK_SIZE 2

// use for defining FREE_LIST_INSERT_STRATEGY compile time value...
#define FILO 0
#define ADDRESS_ORDERED 1

// use for defining MINI_BLOCK_OPTIMIZATION compile time value...
#define FALSE 0
#define TRUE 1

#endif // _MM_H
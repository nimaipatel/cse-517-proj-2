#ifndef _MM_H
#define _MM_H

#include <stdbool.h>
#include <stdlib.h>

void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
void *mm_calloc(size_t nmemb, size_t size);
bool mm_init(void);

#endif // _MM_H
#ifndef _MM_H
#define _MM_H

#include <stdbool.h>
#include <stdlib.h>

void *M_malloc(size_t size);
void M_free(void *ptr);
void *M_realloc(void *ptr, size_t size);
void *M_calloc(size_t nmemb, size_t size);
bool M_init(void);

#endif // _MM_H
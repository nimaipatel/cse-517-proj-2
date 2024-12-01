#ifndef _CSV_H
#define _CSV_H

#include "defines.h"
#include <stdio.h>

FILE *CSV_Open(const Char8 *filename);
void CSV_Write_Header(FILE *f);
void CSV_Close(FILE *f);
void CSV_Write(FILE *f, const Char8 *trace, F64 malloc, F64 malloc_moe, F64 realloc, F64 realloc_moe, F64 free,
               F64 free_moe, F64 total, F64 total_moe, F64 util);

#endif // _CSV_H
#include "csv.h"
#include "defines.h"
#include <stdio.h>

FILE *
CSV_Open(const Char8 *filename)
{
    FILE *f = fopen(filename, "w");
    return f;
}

void
CSV_Write_Header(FILE *f)
{
    fprintf(
        f,
        "trace, malloc mean, malloc MOE, realloc mean, realloc MOE, free mean, free MOE, total mean, total MOE, util\n");
}

void
CSV_Write(FILE *f, const Char8 *trace, F64 malloc, F64 malloc_moe, F64 realloc, F64 realloc_moe, F64 free, F64 free_moe,
          F64 total, F64 total_moe, F64 util)
{
    fprintf(f, "%s, ", trace);
    fprintf(f, "%f, %f, ", malloc, malloc_moe);
    fprintf(f, "%f, %f, ", realloc, realloc_moe);
    fprintf(f, "%f, %f, ", free, free_moe);
    fprintf(f, "%f, %f, ", total, total_moe);
    fprintf(f, "%f\n", util);
}

void
CSV_Close(FILE *f)
{
    if (f) {
        fclose(f);
        f = NULL;
    }
}
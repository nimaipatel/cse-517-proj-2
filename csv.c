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
    if (f)
    {
        fclose(f);
        f = NULL;
    }
}

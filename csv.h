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

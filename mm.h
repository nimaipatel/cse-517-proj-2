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

size_t Linear_Binning(size_t block_size);
size_t Exponential_Binning(size_t block_size);
size_t Hybrid_Binning(size_t block_size);
size_t Range_Binning(size_t block_size);

// use for defining FREE_LIST_INSERT_STRATEGY compile time value...
#define FILO 0
#define ADDRESS_ORDERED 1

// use for defining MINI_BLOCK_OPTIMIZATION compile time value...
#define FALSE 0
#define TRUE 1

#endif // _MM_H

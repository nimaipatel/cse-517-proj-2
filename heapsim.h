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

#ifndef _HEAPSIM_H
#define _HEAPSIM_H

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_HEAP_SIZE (1ull * (1ull << 40)) /* 1 TB */

void Heap_Sim_Init(void);
void Heap_Sim_Release(void);
void *Heap_Sim_Sbrk(intptr_t incr);
void Heap_Sim_Brk(void);
void *Heap_Sim_Get_Low(void);
void *Heap_Sim_Get_High(void);
size_t Heap_Sim_Get_Heap_Size(void);
size_t Heap_Sim_Get_Page_Size(void);

#endif // _HEAPSIM_H

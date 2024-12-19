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

#include "mm.h"

// possible values: integer, 0 = first-fit
#define BEST_FIT_SEARCH_LIMIT 0x10

// possible values: TRUE, FALSE
#define MINI_BLOCK_OPTIMIZATION TRUE

// possible values: ADDRESS_ORDERED, FILO
#define FREE_LIST_INSERT_STRATEGY FILO

// define a free table size and then define the binning strategy in
// Size_Get_Bin_Index(...)
#define FREE_TABLE_SIZE 0x10

// The function takes block_size and returns index of the free list bin it
// should be in.
// Possible values: Linear_Binning, Exponential_Binning, Hybrid_Binning,
// Range_Binning, and you can also define your own function
#define Size_Get_Bin_Index Linear_Binning

// name of the CSV file where statistics will be dumped
#define RUN_NAME "output"

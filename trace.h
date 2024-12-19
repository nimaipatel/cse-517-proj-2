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

#ifndef _TRACE_H
#define _TRACE_H

#include "defines.h"
#include "vec_u64.h"

typedef struct Trace_Op
{
    enum
    {
        ALLOC,
        FREE,
        REALLOC
    } type;
    size_t id;
    size_t size;
} Trace_Op;

typedef struct Trace
{
    size_t num_ids;
    size_t num_ops;
    size_t data_bytes;
    Trace_Op *ops;
} Trace;

typedef struct Trace_Run_Result
{
    Vec_U64 malloc_cyc;
    Vec_U64 realloc_cyc;
    Vec_U64 free_cyc;
    F64 util;
} Trace_Run_Result;

Trace_Run_Result Trace_Run(Trace, U64 perf_type, U64 perf_config);

#endif // _TRACE_H

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

#ifndef _VEC_U64_H
#define _VEC_U64_H

#include <stddef.h>
#include <stdint.h>

typedef struct Vec_U64
{
#define VEC_INIT_CAP 0x10
    size_t len;
    size_t cap;
    uint64_t *data;
} Vec_U64;

typedef struct Vec_U64_Stats_Result
{
    uint64_t sum;
    double mean;
    double variance;
    double stddev;
    double margin_of_error;
} Vec_U64_Stats_Result;

void Vec_U64_Push(Vec_U64 *, uint64_t);
void Vec_U64_Reserve(Vec_U64 *, size_t);
void __Vec_U64_Append(Vec_U64 *, size_t, ...);
#define Vec_U64_Append(vec, ...) \
    __Vec_U64_Append((vec), sizeof((Vec_U64[]){ __VA_ARGS__ }) / sizeof(Vec_U64), __VA_ARGS__)
void Vec_U64_Release(Vec_U64);
Vec_U64_Stats_Result Vec_U64_Stats(Vec_U64);

#endif // _VEC_U64_H

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

#ifndef _STRING_BETTER_H
#define _STRING_BETTER_H

#include "defines.h"

typedef struct String_View
{
    const Char8 *data;
    size_t len;
} String_View;

typedef struct String
{
    Char8 *data;
    size_t len;
    size_t cap;
} String;

String String_Read_File(const Char8 *);
String_View String_Slice(String, size_t start, size_t len);
String_View String_View_From_Cstr(const Char8 *);
void String_Release(String);

#endif // _STRING_BETTER_H

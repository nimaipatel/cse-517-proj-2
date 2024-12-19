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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "defines.h"
#include "string.h"

String
String_Read_File(const Char8 *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        return (String){ 0 };
    }

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    Char8 *data = malloc(len);
    fread(data, 1, len, file);

    return (String){
        .data = data,
        .len = len,
        .cap = len,
    };
}

String
String_Clone(String str)
{
    Char8 *data = malloc(str.len);
    assert(data && "Allocation Failure");
    memcpy(data, str.data, str.len);
    return (String){
        .data = data,
        .len = str.len,
        .cap = str.len,
    };
}

String
String_From_String_View(String_View view)
{
    Char8 *data = malloc(view.len);
    assert(data && "Allocation Failure");
    memcpy(data, view.data, view.len);
    return (String){
        .data = data,
        .len = view.len,
        .cap = view.len,
    };
}

String_View
String_Slice(String str, size_t start, size_t len)
{
    return (String_View){
        .data = str.data + start,
        .len = len,
    };
}

String_View
String_View_From_Cstr(const Char8 *cstr)
{
    return (String_View){ .data = cstr, .len = strlen(cstr) };
}

void
String_Release(String str)
{
    free(str.data);
}

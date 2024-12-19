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

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

#include "defines.h"
#include "string.h"
#include "trace_parser.h"

static Char8
Trace_Parse_Char(String_View input, size_t *index)
{
    Char8 c = input.data[*index];
    *index += 1;
    return c;
}

static void
Trace_Parse_Skip_Whitespace(String_View input, size_t *index)
{
    while (input.data[*index] == ' ' || input.data[*index] == '\n' || input.data[*index] == '\r' ||
           input.data[*index] == '\t')
    {
        *index += 1;
    }
}

static U64
Trace_Parse_U64(String_View input, size_t *index)
{
    U64 result = 0;
    while ('0' <= input.data[*index] && input.data[*index] <= '9')
    {
        result = result * 10 + (input.data[*index] - '0');
        *index += 1;
    }
    return result;
}

static Char8
Trace_Peek_Next_Char(String_View input, size_t *index)
{
    return input.data[*index];
}

static Trace_Op
Trace_Parse_Alloc(String_View input, size_t *index)
{
    Char8 c = Trace_Parse_Char(input, index);
    assert(c == 'a');
    Trace_Parse_Skip_Whitespace(input, index);

    size_t id = Trace_Parse_U64(input, index);
    Trace_Parse_Skip_Whitespace(input, index);

    size_t size = Trace_Parse_U64(input, index);
    Trace_Parse_Skip_Whitespace(input, index);

    return (Trace_Op){ ALLOC, id, size };
}

static Trace_Op
Trace_Parse_Realloc(String_View input, size_t *index)
{
    Char8 c = Trace_Parse_Char(input, index);
    assert(c == 'r');
    Trace_Parse_Skip_Whitespace(input, index);

    size_t id = Trace_Parse_U64(input, index);
    Trace_Parse_Skip_Whitespace(input, index);

    size_t size = Trace_Parse_U64(input, index);
    Trace_Parse_Skip_Whitespace(input, index);

    return (Trace_Op){ REALLOC, id, size };
}

static Trace_Op
Trace_Parse_Free(String_View input, size_t *index)
{
    Char8 c = Trace_Parse_Char(input, index);
    assert(c == 'f');
    Trace_Parse_Skip_Whitespace(input, index);

    size_t id = Trace_Parse_U64(input, index);
    Trace_Parse_Skip_Whitespace(input, index);

    return (Trace_Op){ FREE, id, 0 };
}

Trace
Trace_Parse(String_View input)
{
    size_t index = 0;

    // ignore...
    Trace_Parse_U64(input, &index);
    Trace_Parse_Skip_Whitespace(input, &index);

    const U64 num_ids = Trace_Parse_U64(input, &index);
    Trace_Parse_Skip_Whitespace(input, &index);

    const U64 num_ops = Trace_Parse_U64(input, &index);
    Trace_Parse_Skip_Whitespace(input, &index);

    const U64 data_bytes = Trace_Parse_U64(input, &index);
    Trace_Parse_Skip_Whitespace(input, &index);

    Trace_Op *ops = malloc(num_ops * sizeof(*ops));
    assert(ops && "Allocation Failure");

    size_t op_index = 0;
    U64 max_id = 0;
    while (index < input.len)
    {
        const Char8 c = Trace_Peek_Next_Char(input, &index);
        switch (c)
        {
        case 'a':
        {
            ops[op_index] = Trace_Parse_Alloc(input, &index);
            max_id = MAX(ops[op_index].id, max_id);
            break;
        }

        case 'r':
        {
            ops[op_index] = Trace_Parse_Realloc(input, &index);
            max_id = MAX(ops[op_index].id, max_id);
            break;
        }

        case 'f':
        {
            ops[op_index] = Trace_Parse_Free(input, &index);
            max_id = MAX(ops[op_index].id, max_id);
            break;
        }

        default:
        {
            assert(false && "Unknown trace operation");
        }
        }
        op_index += 1;
    }

    assert(max_id == num_ids - 1);
    assert(op_index == num_ops);

    return (Trace){ num_ids, num_ops, data_bytes, ops };
}

void
Trace_Release(Trace trace)
{
    free(trace.ops);
}

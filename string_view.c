#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "string_view.h"

String_View
String_View_Read_File(const Char8 *path)
{
    FILE *file = fopen(path, "r");
    assert(file && "File not found");

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    Char8 *data = malloc(len + 1);
    assert(data && "Allocation Failure");

    fread(data, 1, len, file);
    data[len] = '\0';

    return (String_View){ .data = data, .len = len };
}
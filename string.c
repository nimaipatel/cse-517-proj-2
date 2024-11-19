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
    if (!file) {
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
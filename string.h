#ifndef _STRING_BETTER_H
#define _STRING_BETTER_H

#include "defines.h"

typedef struct String_View {
    const Char8 *data;
    size_t len;
} String_View;

typedef struct String {
    Char8 *data;
    size_t len;
    size_t cap;
} String;

String String_Read_File(const Char8 *);
String_View String_Slice(String, size_t start, size_t len);
String_View String_View_From_Cstr(const Char8 *);
void String_Release(String);

#endif // _STRING_BETTER_H

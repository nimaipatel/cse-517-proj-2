#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H

#include "defines.h"

typedef struct String_View {
    const Char8 *data;
    size_t len;
} String_View;

String_View String_View_Read_File(const Char8 *path);

#endif // _STRING_VIEW_H

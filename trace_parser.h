#ifndef _TRACE_PARSER_H
#define _TRACE_PARSER_H

#include "defines.h"
#include "string_view.h"
#include "trace.h"

Trace Trace_Parse(String_View input);

#endif // _TRACE_PARSER_H
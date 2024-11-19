#ifndef _TRACE_PARSER_H
#define _TRACE_PARSER_H

#include "defines.h"
#include "string.h"
#include "trace.h"

Trace Trace_Parse(String_View input);
void Trace_Release(Trace trace);

#endif // _TRACE_PARSER_H
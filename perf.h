#ifndef _PERF_H
#define _PERF_H

#include "defines.h"

int Perf_Start(const U64 type, const U64 config);
U64 Perf_Stop(int fd);

#endif // _PERF_H
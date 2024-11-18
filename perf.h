#ifndef _PERF_H
#define _PERF_H

#include <stdint.h>

int Perf_Start(const uint64_t type, const uint64_t config);
uint64_t Perf_Stop(int fd);

#endif // _PERF_H
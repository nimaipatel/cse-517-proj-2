#define _GNU_SOURCE
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>

#include "perf.h"

static long
Perf_Event_Open(struct perf_event_attr *hw_event, pid_t pid, int cpu,
                int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

int
Perf_Start(const uint64_t type, const uint64_t config)
{
    struct perf_event_attr pe = {
        .type = type,
        .size = sizeof(struct perf_event_attr),
        .config = config,
        .disabled = 1,
        .exclude_kernel = 1,
        .exclude_hv = 1,
    };

    const int fd = Perf_Event_Open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe.config);
        exit(EXIT_FAILURE);
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    return fd;
}

uint64_t
Perf_Stop(int fd)
{
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    uint64_t count;
    read(fd, &count, sizeof(uint64_t));
    close(fd);
    return count;
}
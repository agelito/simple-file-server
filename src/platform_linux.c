// platform_linux.c

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200112L

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#include <sys/select.h>

#include "platform.h"

#define NANOSECOND 1000000000

#include "platform_common.c"
#include "console_linux.c"
#include "timer_linux.c"
#include "socket_linux.c"

void
thread_sleep(int milliseconds)
{
	usleep(milliseconds * 1000);
}

double
time_get_seconds()
{
	uint64_t nanoseconds = time_get_nanoseconds();
	double seconds = (double)nanoseconds / NANOSECOND;
	
	return seconds;
}

uint64_t
time_get_nanoseconds()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC, &timespec);

    uint64_t nanoseconds = timespec.tv_sec * NANOSECOND;
    nanoseconds += timespec.tv_nsec;
    return nanoseconds;
}

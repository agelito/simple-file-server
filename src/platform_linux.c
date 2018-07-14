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
#include "selectable_linux.c"
#include "console_linux.c"
#include "timer_linux.c"
#include "socket_linux.c"

int platform_quit = 0;

void
thread_sleep(int milliseconds)
{
	usleep(milliseconds * 1000);
}



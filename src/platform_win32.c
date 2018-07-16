// platform_win32.c

#include "platform.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "platform_common.c"
#include "timer_win32.c"
#include "console_win32.c"
#include "socket_win32.c"
#include "selectable_win32.c"

int platform_quit = 0;

void
thread_sleep(int milliseconds)
{
	Sleep(milliseconds);
}



// console_linux.c

#include "../platform.h"

#include <signal.h>

void
signal_handler(int sig)
{
	platform_quit = 1;
}

void
console_init()
{
	signal(SIGINT, signal_handler);
}

void
console_clear()
{
	printf("\x1b[H\x1b[J");
}

// platform_linux.c

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <ftw.h>
#include <stdarg.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/mman.h>

#include "platform.h"

#define NANOSECOND 1000000000

#include "common/platform_common.c"
#include "linux/selectable_linux.c"
#include "linux/console_linux.c"
#include "linux/timer_linux.c"
#include "linux/socket_linux.c"
#include "linux/filesystem_linux.c"

int platform_quit = 0;
char platform_path_delimiter = '/';

long
platform_format(char* destination, long size, char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(destination, size, format, args);
    va_end(args);

    return (long)result;
}

void
thread_sleep(int milliseconds)
{
	usleep(milliseconds * 1000);
}

void
exe_set_working_directory(char* directory_path)
{
	chdir(directory_path);
}

void
exe_get_directory(char* output, int output_length)
{
	readlink("/proc/self/exe", output, output_length);

	int length = strlen(output);
	int last_character_index = length - 1;
    while(last_character_index > 0)
    {
        char character = *(output + last_character_index);
        if(character == '/') 
        {
            *(output + last_character_index) = 0;
            break;
        }

        --last_character_index;
    }
}

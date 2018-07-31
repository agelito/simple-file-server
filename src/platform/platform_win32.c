// platform_win32.c

#include "platform.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "common/platform_common.c"

#include "win32/timer_win32.c"
#include "win32/console_win32.c"
#include "win32/socket_win32.c"
#include "win32/selectable_win32.c"
#include "win32/filesystem_win32.c"

int platform_quit = 0;
char platform_path_delimiter = '\\';

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
	Sleep(milliseconds);
}

void 
exe_set_working_directory(char* directory_path)
{
    int result = SetCurrentDirectoryA(directory_path);
    if(result == 0) panic("Couldn't set working directory.", 1);
}

void 
exe_get_directory(char* output, int output_length)
{
    int result = GetModuleFileNameA(0, output, output_length);
    if(result == 0) panic("Couldn't get module file name.", 1);

    int last_character_index = result - 1;
    while(last_character_index > 0)
    {
        char character = *(output + last_character_index);
        if(character == '\\') 
        {
            *(output + last_character_index) = 0;
            break;
        }

        --last_character_index;
    }
}

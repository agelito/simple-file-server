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
char platform_path_delimiter = '\\';

void
thread_sleep(int milliseconds)
{
	Sleep(milliseconds);
}

void 
exe_set_working_directory(char* directory_path)
{
    int result = SetCurrentDirectoryA(directory_path);
    if(result == 0) panic(1);
}

void 
exe_get_directory(char* output, int output_length)
{
    int result = GetModuleFileNameA(0, output, output_length);
    if(result == 0) panic(1);

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

void 
filesystem_create_directory(char* directory_path)
{
    int result = CreateDirectoryA(directory_path, 0);
    if(result == 0) panic(1);
}

void 
filesystem_delete_directory(char* directory_path)
{
    int result = RemoveDirectoryA(directory_path);
    if(result == 0) panic(1);
}

int
filesystem_directory_exists(char* directory_path)
{
    DWORD attributes = GetFileAttributesA(directory_path);

    return (attributes != INVALID_FILE_ATTRIBUTES && 
           (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

int
filesystem_open_create_file(char* file_path)
{
    HANDLE file = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(file == INVALID_HANDLE_VALUE) panic(1);
    return (int)file;
}

void 
filesystem_close_file(int file_handle)
{
    CloseHandle((HANDLE)file_handle);
}

void 
filesystem_delete_file(char* file_path)
{
    DeleteFileA(file_path);
}

int
filesystem_file_exists(char* file_path)
{
    DWORD attributes = GetFileAttributesA(file_path);

    return (attributes != INVALID_FILE_ATTRIBUTES && 
            (attributes & FILE_ATTRIBUTE_NORMAL));
}



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

int 
filesystem_open_read_file(char* file_path)
{
    HANDLE file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
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

mapped_file 
filesystem_create_mapped_file(char* file_path, int read_only, int64_t size)
{
    mapped_file mapped;
    mapped.read_only = read_only;

    DWORD mapping_protection = 0;

    file_handle file = 0;
    if(read_only)
    {
        file = filesystem_open_read_file(file_path);
        mapping_protection = PAGE_READONLY;
    }
    else
    {
        file = filesystem_open_create_file(file_path);
        mapping_protection = PAGE_READWRITE;
    }

    mapped.file = file;

    size_t path_length = strlen(file_path);
    mapped.file_path = malloc(sizeof(char) * (path_length + 1));
    strcpy(mapped.file_path, file_path);
    *(mapped.file_path + path_length) = 0;

    LARGE_INTEGER file_size;
    if(size == 0)
    {
        GetFileSizeEx((HANDLE)file, &file_size);
    }
    else
    {
        file_size.QuadPart = size;
    }

    mapped.file_size = file_size.QuadPart;

    file_handle file_mapping = (file_handle)CreateFileMappingA((HANDLE)file, NULL, mapping_protection, 
                                                               file_size.HighPart, file_size.LowPart, NULL);
    if(file_mapping == 0) panic(1);

    mapped.mapping = file_mapping;

    mapped.offset = 0;

    return mapped;
}

void
filesystem_destroy_mapped_file(mapped_file* mapped_file)
{
    if(mapped_file->mapping)
    {
        CloseHandle((HANDLE)mapped_file->mapping);
        mapped_file->mapping = 0;
    }

    if(mapped_file->file)
    {
        CloseHandle((HANDLE)mapped_file->file);
        mapped_file->file = 0;
    }

    if(mapped_file->file_path)
    {
        free(mapped_file->file_path);
        mapped_file->file_path = 0;
    }

    mapped_file->file_size = 0;
    mapped_file->offset    = 0;
}

mapped_file_view
filesystem_file_view_map(mapped_file* mapped_file, int64_t size)
{
    DWORD access = 0;
    if(mapped_file->read_only)
    {
        access = FILE_MAP_READ;
    }
    else
    {
        access = FILE_MAP_ALL_ACCESS;
    }
    
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    if(size > mapped_file->file_size - mapped_file->offset)
    {
        size = mapped_file->file_size - mapped_file->offset;
    }

    int64_t aligned_offset = (mapped_file->offset & ~(system_info.dwAllocationGranularity - 1));

    int64_t extra_size          = (mapped_file->offset - aligned_offset);
    int64_t size_with_alignment = (size + extra_size);

    DWORD offset_high = (aligned_offset >> 32) & 0xffffffff;
    DWORD offset_low  = (aligned_offset        & 0xffffffff);

    mapped_file_view view;
    view.base = MapViewOfFile((HANDLE)mapped_file->mapping, access, offset_high, offset_low, (size_t)size_with_alignment);
    if(view.base == 0) 
    {
        DWORD error_code = GetLastError();
        printf("Error mapping file view: %d\n", error_code);
        panic(1);
    }

    view.mapped       = (uint8_t*)view.base + extra_size;
    view.size_aligned = size_with_alignment;
    view.size         = size;

    return view;
}

void 
filesystem_file_view_unmap(mapped_file_view* view)
{
    UnmapViewOfFile(view->base);
    view->base		   = 0;
    view->mapped	   = 0;
    view->size		   = 0;
    view->size_aligned = 0;
}

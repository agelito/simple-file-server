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

void
filesystem_create_directory(char* directory_path)
{
	mkdir(directory_path, 0700);
}

int
filesystem_delete_directory_callback(const char *path, const struct stat *sb,
                                     int type, struct FTW *ftw)
{
	UNUSED(sb);
	UNUSED(type);
	UNUSED(ftw);
	
    int remove_result = remove(path);
    return remove_result;
}

void
filesystem_delete_directory(char* directory_path)
{
	nftw(directory_path, filesystem_delete_directory_callback, 64, FTW_DEPTH | FTW_PHYS);
}

int
filesystem_directory_exists(char* directory_path)
{
	struct stat dir_stat = {0};
	return (stat(directory_path, &dir_stat) != -1);
}

int
filesystem_open_create_file(char* file_path)
{
	int permissions = S_IRUSR | S_IRGRP | S_IROTH;
	int file_handle = open(file_path, O_RDWR | O_CREAT, permissions);
	return file_handle;
}

int
filesystem_open_read_file(char* file_path)
{
	int file_handle = open(file_path, O_RDONLY);
	return file_handle;
}

void
filesystem_close_file(int file_handle)
{
	close(file_handle);
}

void
filesystem_delete_file(char* file_path)
{
	unlink(file_path);
}

int
filesystem_file_exists(char* file_path)
{
	return (access(file_path, F_OK) != -1);
}

mapped_file
filesystem_create_mapped_file(char* file_path, int read_only, int64_t size)
{
	mapped_file mapped;
	mapped.read_only = read_only;

	file_handle file = 0;
	if(read_only)
	{
		file = filesystem_open_read_file(file_path);
	}
	else
	{
		file = filesystem_open_create_file(file_path);
	}

	mapped.file = file;

	size_t path_length = strlen(file_path);
    mapped.file_path = malloc(sizeof(char) * (path_length + 1));
    strcpy(mapped.file_path, file_path);
    *(mapped.file_path + path_length) = 0;

    int64_t file_size;
    if(size == 0)
    {
	    lseek(file, 0 , SEEK_END);
	    file_size = (int64_t)lseek(file, 0, SEEK_CUR);
	    lseek(file, 0 , SEEK_SET);
    }
    else
    {
	    file_size = size;

	    fallocate(file, 0, 0, (off_t)size);
    }

    mapped.mapping	 = 0;
    mapped.file_size = file_size;
    mapped.offset	 = 0;

    return mapped;
}

void
filesystem_destroy_mapped_file(mapped_file* mapped_file)
{
	close(mapped_file->file);
	mapped_file->file = 0;
	free(mapped_file->file_path);
	mapped_file->file_path = 0;
	mapped_file->file_size = 0;
	mapped_file->offset	   = 0;
}

mapped_file_view
filesystem_file_view_map(mapped_file* mapped_file, int64_t size)
{
	mapped_file_view view;

	int protection = 0;
	if(mapped_file->read_only)
	{
		protection = PROT_READ;
	}
	else
	{
		protection = PROT_READ | PROT_WRITE;
	}

	long page_size = (long)getpagesize();

	int64_t aligned_offset		= (mapped_file->offset & ~(page_size - 1));
	int64_t extra_size			= (mapped_file->offset - aligned_offset);
	int64_t aligned_size        = ((size + extra_size) & ~(page_size - 1));

	if(aligned_size < size)
		aligned_size += page_size;

	view.base = mmap(0, (size_t)aligned_size, protection, MAP_SHARED,
	                 mapped_file->file, (off_t)aligned_offset);
	if(view.base == MAP_FAILED) panic(1);
	view.mapped		  = (uint8_t*)view.base + extra_size;
	view.size_aligned = aligned_size;
	view.size		  = size;

	return view;
}

void
filesystem_file_view_unmap(mapped_file_view* view)
{
	munmap(view->base, view->size_aligned);
	view->base		   = 0;
	view->mapped	   = 0;
	view->size		   = 0;
	view->size_aligned = 0;
}

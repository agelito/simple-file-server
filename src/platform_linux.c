// platform_linux.c

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <ftw.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include "platform.h"

#define NANOSECOND 1000000000

#include "platform_common.c"
#include "selectable_linux.c"
#include "console_linux.c"
#include "timer_linux.c"
#include "socket_linux.c"

int platform_quit = 0;
char platform_path_delimiter = '/';

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

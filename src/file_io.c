// file_io.c

#include <stdio.h>

typedef struct file_io 
{
    int   temporary_file_index;
    char* temporary_directory;
    char* upload_directory;
} file_io;

typedef struct file_io_file 
{
    int  os_file_handle;
    int  bytes_written;
    int  bytes_read;
    char file_name[MAX_FILE_NAME];
} file_io_file;

void
create_temporary_directory(char* output_path, int output_path_length)
{
    platform_format(output_path, output_path_length, "tmp");

    if(!filesystem_directory_exists(output_path))
    {
        filesystem_create_directory(output_path);
    }
}

void
create_upload_directory(char* output_path, int output_path_length)
{
    platform_format(output_path, output_path_length, "upload");

    if(!filesystem_directory_exists(output_path))
    {
        filesystem_create_directory(output_path);
    }
}

file_io 
file_io_initialize()
{
    char exe_directory[MAX_FILE_NAME];
    exe_get_directory(exe_directory, MAX_FILE_NAME);
    exe_set_working_directory(exe_directory);

    file_io io;
    io.temporary_file_index = 0;
    io.temporary_directory = malloc(MAX_FILE_NAME);
    create_temporary_directory(io.temporary_directory, MAX_FILE_NAME);
    io.upload_directory = malloc(MAX_FILE_NAME);
    create_upload_directory(io.upload_directory, MAX_FILE_NAME);

    return io;
}

void
file_io_destroy(file_io* io)
{
    if(io->temporary_directory)
    {
        filesystem_delete_directory(io->temporary_directory);

        free(io->temporary_directory);
        io->temporary_directory = 0;
    }

    if(io->upload_directory)
    {
        free(io->upload_directory);
        io->upload_directory = 0;
    }

    io->temporary_file_index = 0;
}

void
generate_temporary_file_path(file_io* io, char* output_path, int output_path_length)
{
    platform_format(output_path, output_path_length, "%s%c%05d", io->temporary_directory,
                    platform_path_delimiter, io->temporary_file_index++);
}

file_io_file
file_io_create_file(char* path)
{
    file_io_file file;

    file.bytes_written = 0;
    file.bytes_read = 0;

    platform_format(file.file_name, MAX_FILE_NAME, "%s", path);

    file.os_file_handle = filesystem_open_create_file(path);

    return file;
}

void
file_io_destroy_file(file_io_file* file)
{
    if(file->os_file_handle)
    {
        filesystem_close_file(file->os_file_handle);
        file->os_file_handle = 0;
    }

    file->bytes_written = 0;
    file->bytes_read = 0;
}

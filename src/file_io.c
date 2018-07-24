// file_io.c

#include <stdio.h>

typedef struct file_io 
{
    int   temporary_file_index;
    char* temporary_directory;
    char* upload_directory;
} file_io;

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
create_upload_file_path(file_io* io, char* file_name, char* output_path, int output_path_length)
{
    platform_format(output_path, output_path_length, "%s%c%s", io->upload_directory,
                    platform_path_delimiter, file_name);
}

void
generate_temporary_file_path(file_io* io, char* output_path, int output_path_length)
{
    platform_format(output_path, output_path_length, "%s%c%05d", io->temporary_directory,
                    platform_path_delimiter, io->temporary_file_index++);
}

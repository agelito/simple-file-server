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

void
file_io_copy_file(mapped_file* source, char* destination, int64_t file_size)
{
    int copy_byte_count = 4096;

    mapped_file destination_file = filesystem_create_mapped_file(destination, 0, file_size);
    // TODO: Check for errors opening destination file.

    int64_t bytes_copied = 0;
    while(bytes_copied < file_size)
    {
        int64_t remaining_bytes = (file_size - bytes_copied);
        int     copy_bytes      = copy_byte_count;

        if(copy_bytes > remaining_bytes)
        {
            copy_bytes = (int)remaining_bytes;
        }

        mapped_file_view sview = filesystem_file_view_map(source, copy_bytes);
        mapped_file_view dview = filesystem_file_view_map(&destination_file, copy_bytes);
        // TODO: Check for errors mapping views.

        memcpy(dview.mapped, sview.mapped, copy_bytes);
        
        filesystem_file_view_unmap(&sview);
        filesystem_file_view_unmap(&dview);

        bytes_copied += copy_bytes;
    }

    filesystem_destroy_mapped_file(&destination_file);
}

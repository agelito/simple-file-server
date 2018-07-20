// map_file_test.c

#include "platform.h"

int 
main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("usage: %s <file>\n", argv[0]);
        return 0;
    }

    printf("mapping file: %s\n", argv[1]);

    mapped_file file = filesystem_create_mapped_file(argv[1], 1, 0);
    printf("mapped file of size: %lld\n", file.file_size);

    printf("create view of %lld bytes\n", file.file_size / 8);
    mapped_file_view view = filesystem_file_view_map(&file, file.file_size / 8);

    printf("view base: %llp\n", view.base);
    printf("view mapped: %llp\n", view.mapped);

    filesystem_file_view_unmap(&view);

    printf("set offset to: %lld\n", file.file_size / 2);
    file.offset = file.file_size / 2;

    printf("create view of %lld bytes\n", file.file_size / 8);
    view = filesystem_file_view_map(&file, file.file_size / 8);

    printf("view base: %llp\n", view.base);
    printf("view mapped: %llp\n", view.mapped);

    filesystem_file_view_unmap(&view);
    filesystem_destroy_mapped_file(&file);

    return 0;
}

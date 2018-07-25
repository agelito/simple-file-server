// map_file_test.c

#include "platform/platform.h"

void
map_view_of_file(mapped_file* file, int64_t offset, int64_t size)
{
	printf("set offset to: %jd\n", offset);
	file->offset = offset;
	
	printf("create view of %jd bytes\n", size);
	mapped_file_view view = filesystem_file_view_map(file, size);

	printf("view base: %p\n", view.base);
	printf("view mapped: %p\n", view.mapped);

	filesystem_file_view_unmap(&view);
}

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
    printf("mapped file of size: %jd\n", file.file_size);

    map_view_of_file(&file, 0,                     file.file_size / 4);
    map_view_of_file(&file, file.file_size / 2,    file.file_size / 4);
    map_view_of_file(&file, file.file_size - 1024, file.file_size / 4);

    filesystem_destroy_mapped_file(&file);

    return 0;
}

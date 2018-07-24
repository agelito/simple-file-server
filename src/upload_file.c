// upload_file.c

#include "platform.h"
#include "protocol.h"

#include "connection.c"
#include "string_sanitize.c"

void
upload_file(connection* connection, mapped_file* file)
{
    int chunk_size  = 1024;

    packet_file_upload_begin upload_begin;
    upload_begin.file_size        = file->file_size;
    upload_begin.file_name_length = (int)strlen(file->file_path);

    connection_push_data_packet(connection, PACKET_FILE_UPLOAD_BEGIN, (char*)&upload_begin,
                                sizeof(upload_begin), file->file_path,
                                (short)upload_begin.file_name_length);

    int64_t sent_bytes = 0;

    int upload_in_progress = 1;
    while(upload_in_progress)
    {
        if(sent_bytes < file->file_size)
        {
            int send_chunk_size = chunk_size;
            int64_t remaining_bytes = (file->file_size - sent_bytes);
            if(send_chunk_size > remaining_bytes)
            {
                send_chunk_size = remaining_bytes;
            }
            packet_file_upload_chunk upload_chunk;
            upload_chunk.chunk_size = send_chunk_size;

            file->offset = sent_bytes;
            mapped_file_view view = filesystem_file_view_map(file, send_chunk_size);

            connection_push_data_packet(connection, PACKET_FILE_UPLOAD_CHUNK, (char*)&upload_chunk,
                                        sizeof(upload_chunk), view.mapped, (short)send_chunk_size);

            sent_bytes += send_chunk_size;

            filesystem_file_view_unmap(&view);
        }
        else
        {
	        printf("finished sending\n");
	        
            connection_push_empty_packet(connection, PACKET_FILE_UPLOAD_FINAL);
            upload_in_progress = 0;
        }

        if(connection->send_data_count > 0)
        {
	        printf("sending network data: %d\n", connection->send_data_count);
	        connection_send_network_data(connection);
        }
    }
}

int 
main(int argc, char* argv[])
{
    if(argc < 3)
    {
        printf("usage: %s <server> <file>\n", argv[0]);
        return 0;
    }

    printf("using server: %s:%d\n", argv[1], DEFAULT_LISTEN_PORT);
    printf("uploading file: %s\n", argv[2]);

	socket_initialize();

    connection_storage connections = create_connection_storage(1);

    socket_address server_address = socket_create_inet_address(argv[1], DEFAULT_LISTEN_PORT);

    printf("creating socket.\n");

    socket_handle socket = socket_create_tcp();
    socket_set_blocking(socket);

    connection* connection = create_new_connection(&connections);
    connection->socket = socket;
    connection->address = server_address;
    connection->socket_initialized = 1;

    socket_address_to_string(&server_address, connection->address_string, INET_STRADDR_LENGTH);

    printf("connecting socket.\n");

    socket_connect(connection->socket, &server_address);

    printf("create memory-mapped file: %s\n", argv[2]);

    mapped_file file = filesystem_create_mapped_file(argv[2], 1, 0);

    upload_file(connection, &file);

    filesystem_destroy_mapped_file(&file);

    connection_disconnect(connection);

    remove_connection_index(&connections, 0);
    destroy_connection_storage(&connections);

	socket_cleanup();

    return 0;
}

// fileserver.c

#include "platform/platform.h"
#include "protocol.h"

#include "application.h"
#include "string_sanitize.c"
#include "file_io.c"
#include "connection.c"

#define TARGET_UPS 100

typedef struct connection_statistics
{
	int connections;
	int rejected_connections;
    int pending_disconnections;
	int disconnections;
    int sent_bytes;
    int recv_bytes;
} connection_statistics;

typedef struct fileserver
{
	socket_handle         socket;
    socket_address        address;
	char                  address_string[INET_STRADDR_LENGTH];
	selectable_set        selectable;
	connection_storage    connection_storage;
    connection_statistics statistics;
    file_io               io;
} fileserver;

#define TO_MB(bytes) (float)bytes / 1024.0f / 1024.0f

void
print_server_info(fileserver* fileserver, timer* timer)
{
    connection_statistics* statistics = &fileserver->statistics;

    console_clear();

    printf("%-20s: %s\n", "Listen address", fileserver->address_string);

    float average = (float)1 / timer->frame_counter;
    printf("%-20s: %.02fms %.02fups %.02fmcy %.02fs\n", "Performance", 
           timer->delta_milliseconds * average, 
           timer->frames_per_second * average, 
           timer->megacycles_per_frame * average,
           timer->elapsed_seconds);

    if(statistics->connections)
        printf("%-20s: %d\n", "Connected", statistics->connections);
    if(statistics->disconnections)
        printf("%-20s: %d\n", "Disconnected", statistics->disconnections);
    if(statistics->rejected_connections)
        printf("%-20s: %d\n" ,"Rejected", statistics->rejected_connections);
    if(statistics->pending_disconnections)
        printf("%-20s: %d\n", "Pending Disconnections", statistics->pending_disconnections);
    if(statistics->sent_bytes)
        printf("%-20s: %.02fMB/u %.02fMB/s\n", "Outgoing", 
               TO_MB(statistics->sent_bytes * average), TO_MB(statistics->sent_bytes));
    if(statistics->recv_bytes)
        printf("%-20s: %.02fMB/u %.02fMB/s\n", "Incoming", 
               TO_MB(statistics->recv_bytes * average), TO_MB(statistics->recv_bytes));

    statistics->connections			 = 0;
    statistics->disconnections		 = 0;
    statistics->rejected_connections = 0;
    statistics->sent_bytes           = 0;
    statistics->recv_bytes           = 0;
        
    connection_storage* connection_storage = &fileserver->connection_storage;
    printf("%-20s: %d / %d\n", "Connections", connection_storage->count,
           connection_storage->capacity);

    // TODO: Need to improve performance of outputting transfer progress bars.

    int   progress_bar_width  = 60;
    char* progress_bar_string = (char*)malloc(progress_bar_width + 1);

    int i;
    for(i = 0; i < connection_storage->count; ++i)
    {
        connection* connection = (connection_storage->connections + i);
        if(connection->transfer_in_progress)
        {
            connection_file_transfer* transfer = &connection->transfer;

            float transfer_percent = (float)connection->transfer.byte_count_recv /
                connection->transfer.file_size;

            printf("%-20s: ", "Transfer");

            printf("|");

            int progress_bar_head = (int)(progress_bar_width * transfer_percent);
		        
            int c;
            for(c = 0; c < progress_bar_width; ++c)
            {
                char character = '-';
                if(c < progress_bar_head)
                {
                    character = '=';
                }
                else if(c == progress_bar_head)
                {
                    character = '>';
                }

                *(progress_bar_string + c) = character;
            }

            *(progress_bar_string + progress_bar_width) = 0;
            printf("%s", progress_bar_string);


            printf("|");

            printf(" %.02f / %.02f MB (%0.02f%%)\n", TO_MB(transfer->byte_count_recv), 
                   TO_MB(transfer->file_size), transfer_percent * 100.0f);
        }
    }

    free(progress_bar_string);
}

void
fileserver_upload_begin(connection* connection, char* packet_body, int body_length)
{
	if(body_length < sizeof(packet_file_upload_begin))
	{
        connection_protocol_error(connection);
		return;
	}
	
	if(connection->transfer_in_progress)
	{
        connection_protocol_error(connection);
		return;
	}

	packet_file_upload_begin* upload_begin = (packet_file_upload_begin*)packet_body;
	
	int packet_size_with_filename = sizeof(packet_file_upload_begin) + upload_begin->file_name_length;
	if(body_length != packet_size_with_filename)
	{
        connection_protocol_error(connection);
		return;
	}

    connection_transfer_prepare(&connection->transfer, upload_begin->file_size);

    char* packet_file_name = (char*)(packet_body + sizeof(packet_file_upload_begin));
    
    sanitize_file_name(packet_file_name, upload_begin->file_name_length,
                        connection->transfer.file_name_final, MAX_FILE_NAME);

	connection->transfer_in_progress = 1;
    connection->transfer_completed   = 0;
}

void
fileserver_upload_chunk(connection* connection, char* packet_body, int body_length)
{
	if(body_length < sizeof(packet_file_upload_chunk))
	{
		connection_protocol_error(connection);
		return;
	}
	
	if(!connection->transfer_in_progress)
	{
		connection_protocol_error(connection);
		return;
	}

	packet_file_upload_chunk* upload_chunk = (packet_file_upload_chunk*)packet_body;

	int packet_size_with_chunk = sizeof(packet_file_upload_chunk) + upload_chunk->chunk_size;
	if(body_length != packet_size_with_chunk)
	{
        connection_protocol_error(connection);
		return;
	}

    char* chunk_data = (packet_body + sizeof(packet_file_upload_chunk));

    int remaining_io_bytes = (connection->transfer.io_buffer_capacity - 
                              connection->transfer.io_buffer_size);
    if(upload_chunk->chunk_size < remaining_io_bytes)
    {
        memcpy(connection->transfer.io_buffer + connection->transfer.io_buffer_size,
               chunk_data, upload_chunk->chunk_size);
        connection->transfer.io_buffer_size	 += upload_chunk->chunk_size;
        connection->transfer.byte_count_recv += upload_chunk->chunk_size;
    }
}

void
fileserver_upload_final(connection* connection, char* packet_body, int body_length)
{
    UNUSED(packet_body);

    if(body_length != 0)
    {
        connection_protocol_error(connection);
		return;
    }

    if(!connection->transfer_in_progress)
    {
        connection_protocol_error(connection);
		return;
    }

    if(connection->transfer.file_size != connection->transfer.byte_count_recv)
    {
	    connection_protocol_error(connection);
	    return;
    }

    connection->transfer_completed = 1;
}

void 
fileserver_receive_packet_body(connection* connection, int packet_type, char* packet_body, int body_length)
{
    switch(packet_type)
    {
    case PACKET_DISCONNECT: 
        connection->request_disconnect = 1;
        break;
    case PACKET_FILE_UPLOAD_BEGIN:
	    fileserver_upload_begin(connection, packet_body, body_length);
        break;
    case PACKET_FILE_UPLOAD_CHUNK:
	    fileserver_upload_chunk(connection, packet_body, body_length);
	    break;
    case PACKET_FILE_UPLOAD_FINAL:
	    fileserver_upload_final(connection, packet_body, body_length);
	    break;
    default: 
        connection_protocol_error(connection);
        break;
    }
}

int
fileserver_receive_packets(connection* connection, char* data, int length)
{
	int read_bytes = 0;
    while(read_bytes < length)
    {
	    char* read_data = data + read_bytes;
	    
	    int remaining_bytes = (length - read_bytes);
	    
	    int header_length = sizeof(packet_header);
	    if(header_length > remaining_bytes)
	    {
            // NOTE: Packet is incomplete.
		    break;
	    }
	    
	    packet_header* header = (packet_header*)read_data;
        if(header->packet_type == PACKET_INVALID_PROTOCOL)
        {
	        connection->pending_disconnect = 1;
	        break;
        }

        read_bytes      += header_length;
        remaining_bytes -= header_length;

        int packet_length = header->packet_size;
        if(packet_length > remaining_bytes)
        {
            // NOTE: Packet is incomplete.
            read_bytes -= header_length;
            break;
        }

        char* packet_data_body = read_data + header_length;
        fileserver_receive_packet_body(connection, header->packet_type,
                                       packet_data_body, packet_length);

        read_bytes += packet_length;
    }

    return read_bytes;
}

void
accept_incoming_connections(int count, socket_handle socket, connection_storage* connection_storage,
                            connection_statistics* statistics)
{
    UNUSED(count);

    int accepting = 1;
	while(accepting)
	{
		socket_address accepted_address;
		socket_handle accepted = socket_accept(socket, &accepted_address);
		if(accepted == 0 || accepted == -1) break;

		connection* new_connection  = create_new_connection(connection_storage);
		if(new_connection != 0)
		{
			new_connection->socket  = accepted;
			new_connection->address = accepted_address;
			new_connection->socket_initialized = 0;
            
			socket_address_to_string(&accepted_address, new_connection->address_string,
			                         INET_STRADDR_LENGTH);

			statistics->connections += 1;
		}
		else
		{
			socket_close(accepted);
			statistics->rejected_connections += 1;
		}
	}
}

int
process_connection_connections(connection_storage* connection_storage, selectable_set* selectable,
                               connection_statistics* statistics)
{
    int pending_disconnect_count = 0;
	int highest_handle = 0;
	int connection_index;
	for(connection_index = 0; connection_index < connection_storage->count; ++connection_index)
	{
		connection* connection = (connection_storage->connections + connection_index);

        // TODO: Make sure clients can't abuse this to make their connection persistent.
        int transfer_in_progress = ((connection->transfer_in_progress && connection->recv_data_count > 0) ||
                                    connection->transfer_completed);

        if(connection->request_disconnect || connection->error)
        {
            connection_disconnect(connection);
            statistics->disconnections += 1;
        }
		else if(connection->pending_disconnect)
		{
            pending_disconnect_count += 1;

            if(!transfer_in_progress && connection->send_data_count == 0)
            {
                connection_disconnect(connection);
                statistics->disconnections += 1;
            }
		}
            
		if(!connection->socket)
		{
			remove_connection_index(connection_storage, connection_index--);
			continue;
		}

        if(transfer_in_progress || !connection->pending_disconnect)
        {
            selectable_set_set_read(selectable, connection->socket);
        }

		// NOTE: Keep sending data even if connection is pending disconnect.
        if(connection->send_data_count > 0)
        {
            selectable_set_set_write(selectable, connection->socket);
        }

		if((int)connection->socket > highest_handle)
			highest_handle = (int)connection->socket;
	}

    statistics->pending_disconnections = pending_disconnect_count;

	return highest_handle;
}

void
process_connection_network_io(connection_storage* connection_storage, selectable_set* selectable, 
                              connection_statistics* statistics)
{
	int connection_index;
	for(connection_index = 0; connection_index < connection_storage->count; ++connection_index)
	{
		connection* connection = (connection_storage->connections + connection_index);

		if(selectable_set_can_write(selectable, connection->socket))
		{
            if(!connection->socket_initialized)
            {
                socket_set_nonblocking(connection->socket);
                connection->socket_initialized = 1;
            }

			int sent_bytes = connection_send_network_data(connection);
            if(sent_bytes > 0) statistics->sent_bytes += sent_bytes;
		}

		if(selectable_set_can_read(selectable, connection->socket))
		{
            if(!connection->socket_initialized)
            {
                socket_set_nonblocking(connection->socket);
                connection->socket_initialized = 1;
            }

            int recv_buffer_remaining = (connection->recv_data_capacity - connection->recv_data_count);
            char* recv_data_buffer = (connection->recv_data + connection->recv_data_count);
            int recv_bytes = connection_recv_network_data(connection, recv_data_buffer , recv_buffer_remaining);
            if(recv_bytes > 0)
            {
                statistics->recv_bytes += recv_bytes;
                connection->recv_data_count += recv_bytes;
            }

            if(connection->recv_data_count > 0)
            {
				int read_bytes = fileserver_receive_packets(connection, connection->recv_data, connection->recv_data_count);
                if(read_bytes <= connection->recv_data_count)
                {
                    char* offset = (connection->recv_data + read_bytes);
                    int   length = (connection->recv_data_count - read_bytes);
                    if(length > 0)
                    {
                        memmove(connection->recv_data, offset, length);
                    }
                    connection->recv_data_count -= read_bytes;
                }
                else
                {
                    // NOTE: Packet stream error.
                    connection->pending_disconnect = 1;
                    connection->recv_data_count    = 0;
                }
            }
		}
	}
}

void
fileserver_write_final_file(file_io* io, connection_file_transfer* transfer)
{
    // TODO: Optimize file IO. Perhaps need to move to separate thread
    // so touching the disk doesn't block the main thread.

    char upload_path[MAX_FILE_NAME];
    create_upload_file_path(io, transfer->file_name_final, upload_path, MAX_FILE_NAME);

    file_io_copy_file(&transfer->download_file, upload_path, transfer->file_size);
}

void
fileserver_write_downloaded_data(file_io* io, connection* connection)
{
    // TODO: Optimize file IO. Perhaps need to move to separate thread
    // so touching the disk doesn't block the main thread.

    connection_file_transfer* transfer = &connection->transfer;

    if(!transfer->download_file.file_path)
    {
        char temporary_upload_file[MAX_FILE_NAME];
        generate_temporary_file_path(io, temporary_upload_file, MAX_FILE_NAME);

        transfer->download_file = filesystem_create_mapped_file(temporary_upload_file, 0, transfer->file_size);
    }

    if(transfer->download_file.file && transfer->io_buffer_size > 0)
    {
        transfer->download_file.offset = transfer->byte_count_disk;

        mapped_file_view view = filesystem_file_view_map(&transfer->download_file, transfer->io_buffer_size);
        memcpy(view.mapped, transfer->io_buffer, transfer->io_buffer_size);
        filesystem_file_view_unmap(&view);

        transfer->byte_count_disk += transfer->io_buffer_size;

        transfer->io_buffer_size = 0;
    }

    if(connection->transfer_completed)
    {
        if(transfer->byte_count_disk == transfer->file_size)
        {
            fileserver_write_final_file(io, transfer);
        }

        connection_transfer_free(&connection->transfer);
    
        connection->transfer_completed   = 0;
        connection->transfer_in_progress = 0;
    }
}

int
fileserver_tick(void* state, timer* timer)
{
    UNUSED(timer);

    fileserver* fileserver_state = (fileserver*)state;
	selectable_set_clear(&fileserver_state->selectable);

	int highest_handle = process_connection_connections(&fileserver_state->connection_storage,
	                                                    &fileserver_state->selectable,
	                                                    &fileserver_state->statistics);

	int selected = selectable_set_select_noblock(&fileserver_state->selectable, highest_handle);
	if(selected > 0)
	{
		// NOTE: Process already connected connections before accepting new connections.
		process_connection_network_io(&fileserver_state->connection_storage, &fileserver_state->selectable, &fileserver_state->statistics);
	}

	selectable_set_clear(&fileserver_state->selectable);
	selectable_set_set_read(&fileserver_state->selectable, fileserver_state->socket);

	highest_handle = fileserver_state->socket + 1;
	selected = selectable_set_select_noblock(&fileserver_state->selectable, highest_handle);

	if(selected > 0 && selectable_set_can_read(&fileserver_state->selectable, fileserver_state->socket))
	{
		accept_incoming_connections(selected, fileserver_state->socket, &fileserver_state->connection_storage, 
                                    &fileserver_state->statistics);
	}

    for(int i = 0; i < fileserver_state->connection_storage.count; ++i)
    {
        connection* connection = (fileserver_state->connection_storage.connections + i);
        
        if(connection->transfer_in_progress)
        {
            fileserver_write_downloaded_data(&fileserver_state->io, connection);
        }
    }

    return 1;
}

void*
fileserver_create(char* command_line[], int command_line_count)
{
    UNUSED(command_line);
    UNUSED(command_line_count);

    fileserver* fileserver_state = (fileserver*)malloc(sizeof(fileserver));
    memset(fileserver_state, 0, sizeof(fileserver));

    fileserver_state->io = file_io_initialize();

    fileserver_state->selectable          = selectable_set_create();
    fileserver_state->connection_storage  = create_connection_storage(MAX_CONNECTION_COUNT);

    fileserver_state->socket = socket_create_tcp();
    socket_set_nonblocking(fileserver_state->socket);

    fileserver_state->address = socket_create_inet_address("0.0.0.0", DEFAULT_LISTEN_PORT);
    socket_bind(fileserver_state->socket, fileserver_state->address);

    socket_address_to_string(&fileserver_state->address, fileserver_state->address_string, INET_STRADDR_LENGTH);

    socket_listen(fileserver_state->socket);

    return fileserver_state;
}

void
fileserver_destroy(void* state)
{
    fileserver* fileserver_state = (fileserver*)state;
    if(fileserver_state != 0)
    {
        file_io_destroy(&fileserver_state->io);
        destroy_connection_storage(&fileserver_state->connection_storage);
        socket_close(fileserver_state->socket);
        free(fileserver_state);
    }
}

int 
main(int argc, char* argv[]) 
{
    application application;
    application.target_ups = TARGET_UPS;
    application.create = fileserver_create;
    application.destroy = fileserver_destroy;
    application.tick = fileserver_tick;
    application.print = print_server_info;

    application_run(&application, argc, argv);

    printf("Server terminated gracefully.\n");
}

// fileserver.c

#include "platform.h"
#include "protocol.h"

#include "string_sanitize.c"
#include "file_io.c"
#include "connection.c"

#define TARGET_UPS 400

typedef struct connection_statistics
{
	int connections;
	int rejected_connections;
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
	char*                 receive_data_buffer;
	timer                 timer;
    float                 last_print_time;
    connection_statistics statistics;
    file_io               io;
} fileserver;

float
print_server_info(fileserver* fileserver)
{
    timer* timer = &fileserver->timer;
    float last_print_time = fileserver->last_print_time;
	if(last_print_time <= 0.0f || (timer->elapsed_seconds - last_print_time) >= 1.0f)
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
        timer_reset_accumulators(timer);

        if(statistics->connections)
	        printf("%-20s: %d\n", "Connected", statistics->connections);
        if(statistics->disconnections)
	        printf("%-20s: %d\n", "Disconnected", statistics->disconnections);
        if(statistics->rejected_connections)
	        printf("%-20s: %d\n" ,"Rejected", statistics->rejected_connections);
        if(statistics->sent_bytes)
            printf("%-20s: %.02fb/u %db/s\n", "Outgoing", 
                   (float)statistics->sent_bytes * average, statistics->sent_bytes);
        if(statistics->recv_bytes)
            printf("%-20s: %.02fb/u %db/s\n", "Incoming", 
                   (float)statistics->recv_bytes * average, statistics->recv_bytes);

        statistics->connections			 = 0;
        statistics->disconnections		 = 0;
        statistics->rejected_connections = 0;
        statistics->sent_bytes           = 0;
        statistics->recv_bytes           = 0;
        
        connection_storage* connection_storage = &fileserver->connection_storage;
        printf("%-20s: %d / %d\n", "Connections", connection_storage->count,
               connection_storage->capacity);

        last_print_time = (float)timer->elapsed_seconds;
    }

    return last_print_time;
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

    connection_transfer_prepare(&connection->transfer, upload_begin->file_size, upload_begin->chunk_count);

    char* packet_file_name = (char*)(packet_body + sizeof(packet_file_upload_begin));
    santitize_file_name(packet_file_name, upload_begin->file_name_length,
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
        memcpy(connection->transfer.io_buffer + connection->transfer.io_buffer_size, chunk_data, upload_chunk->chunk_size);
        connection->transfer.io_buffer_size += upload_chunk->chunk_size;

        connection->transfer.chunk_completed++;
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

    if(connection->transfer.chunk_count != connection->transfer.chunk_completed)
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
        connection->pending_disconnect = 1;
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

void
fileserver_receive_packets(connection* connection, char* data, int length)
{
    int remaining_bytes = length;
    while(remaining_bytes > 0 && !connection->pending_disconnect)
    {
        packet_header* header = (packet_header*)data;
        if(header->packet_size > remaining_bytes || header->packet_type == PACKET_INVALID_PROTOCOL)
        {
            connection->pending_disconnect = 1;
            break;
        }

        char* packet_data_body = (char*)header + sizeof(packet_header);
        fileserver_receive_packet_body(connection, header->packet_type, packet_data_body,
                                       header->packet_size - sizeof(packet_header));

        data = (data + header->packet_size);
        remaining_bytes -= header->packet_size;
    }
}

void
accept_incoming_connections(int count, socket_handle socket, connection_storage* connection_storage,
                            connection_statistics* statistics)
{
	int max_incoming_per_frame = 128;
	if(count > max_incoming_per_frame)
		count = max_incoming_per_frame;
	
	while(0 < count--)
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
	int highest_handle = 0;
	int connection_index;
	for(connection_index = 0; connection_index < connection_storage->count; ++connection_index)
	{
		connection* connection = (connection_storage->connections + connection_index);

		if(connection->pending_disconnect && connection->send_bytes == 0)
		{
			connection_disconnect(connection);
			statistics->disconnections += 1;
		}
            
		if(!connection->socket)
		{
			remove_connection_index(connection_storage, connection_index--);
			continue;
		}

		if(!connection->pending_disconnect)
		{
			selectable_set_set_read(selectable, connection->socket);
		}

		// NOTE: Keep sending data even if connection is pending disconnect.
        if(connection->send_bytes > 0)
        {
            selectable_set_set_write(selectable, connection->socket);
        }

		if((int)connection->socket > highest_handle)
			highest_handle = (int)connection->socket;
	}

	return highest_handle;
}

void
process_connection_network_io(connection_storage* connection_storage, selectable_set* selectable,
                              char* io_buffer, int io_buffer_size, connection_statistics* statistics)
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
            
            int recv_bytes = connection_recv_network_data(connection, io_buffer, io_buffer_size);
            if(recv_bytes > 0)
            {
                statistics->recv_bytes += recv_bytes;
				fileserver_receive_packets(connection, io_buffer, recv_bytes);
            }
		}
	}
}

void
fileserver_write_final_file(file_io* io, connection_file_transfer* transfer)
{
    char upload_path[MAX_FILE_NAME];
    create_upload_file_path(io, transfer->file_name_final, upload_path, MAX_FILE_NAME);

    mapped_file final_file = filesystem_create_mapped_file(upload_path, 0, transfer->file_size);

    int bytes_copied = 0;
    
    while(bytes_copied < transfer->file_size)
    {
        int copy_bytes = 4096;
        if(copy_bytes > (transfer->file_size - bytes_copied))
        {
            copy_bytes = (transfer->file_size - bytes_copied);
        }

        transfer->download_file.offset = bytes_copied;
        final_file.offset              = bytes_copied;

        mapped_file_view source      = filesystem_file_view_map(&transfer->download_file, copy_bytes);
        mapped_file_view destination = filesystem_file_view_map(&final_file, copy_bytes);

        memcpy(destination.mapped, source.mapped, copy_bytes);

        filesystem_file_view_unmap(&source);
        filesystem_file_view_unmap(&destination);

        bytes_copied += copy_bytes;
    }
    

    filesystem_destroy_mapped_file(&final_file);
}

void
fileserver_write_downloaded_data(file_io* io, connection* connection)
{
    connection_file_transfer* transfer = &connection->transfer;

    if(!transfer->download_file.file_path)
    {
        char temporary_upload_file[MAX_FILE_NAME];
        generate_temporary_file_path(io, temporary_upload_file, MAX_FILE_NAME);

        transfer->download_file = filesystem_create_mapped_file(temporary_upload_file, 0, transfer->file_size);
    }

    if(transfer->download_file.file && transfer->io_buffer_size > 0)
    {
        transfer->download_file.offset = transfer->bytes_done;

        mapped_file_view view = filesystem_file_view_map(&transfer->download_file, transfer->io_buffer_size);
        memcpy(view.mapped, transfer->io_buffer, transfer->io_buffer_size);
        filesystem_file_view_unmap(&view);

        transfer->bytes_done += transfer->io_buffer_size;

        transfer->io_buffer_size = 0;
    }

    if(connection->transfer_completed)
    {
        if(transfer->file_size == transfer->bytes_done)
        {
            fileserver_write_final_file(io, transfer);
        }

        connection_transfer_free(&connection->transfer);
    
        connection->transfer_completed   = 0;
        connection->transfer_in_progress = 0;
    }
}

void
wait_for_target_ups(measure_time* measure, double target_delta)
{
    measure_tick(measure);

    int sleep_epsilon = 2;

    double accumulator = 0.0;
    while(accumulator < target_delta)
    {
        accumulator += measure->delta_time;

        double remaining = (target_delta - accumulator);
        
        int remaining_milliseconds = (int)(remaining * 1000);
        if(remaining_milliseconds > sleep_epsilon)
        {
            thread_sleep(remaining_milliseconds);
        }

        measure_tick(measure);
    }
}

void
fileserver_tick(fileserver* fileserver)
{
	fileserver->last_print_time = print_server_info(fileserver);

	selectable_set_clear(&fileserver->selectable);

	int highest_handle = process_connection_connections(&fileserver->connection_storage,
	                                                    &fileserver->selectable,
	                                                    &fileserver->statistics);

	int selected = selectable_set_select_noblock(&fileserver->selectable, highest_handle);
	if(selected > 0)
	{
		// NOTE: Process already connected connections before accepting new connections.
		process_connection_network_io(&fileserver->connection_storage, &fileserver->selectable,
		                              fileserver->receive_data_buffer,
		                              MAX_PACKET_SIZE, &fileserver->statistics);
	}

	selectable_set_clear(&fileserver->selectable);
	selectable_set_set_read(&fileserver->selectable, fileserver->socket);

	highest_handle = fileserver->socket + 1;
	selected = selectable_set_select_noblock(&fileserver->selectable, highest_handle);

	if(selected > 0 && selectable_set_can_read(&fileserver->selectable, fileserver->socket))
	{
		accept_incoming_connections(selected, fileserver->socket, &fileserver->connection_storage, 
                                    &fileserver->statistics);
	}

    for(int i = 0; i < fileserver->connection_storage.count; ++i)
    {
        connection* connection = (fileserver->connection_storage.connections + i);
        
        if(connection->transfer_in_progress)
        {
            fileserver_write_downloaded_data(&fileserver->io, connection);
        }
    }
}

fileserver
fileserver_create()
{
    fileserver fileserver;
    memset(&fileserver, 0, sizeof(fileserver));

    timer_initialize(&fileserver.timer);

    fileserver.io = file_io_initialize();

    fileserver.selectable          = selectable_set_create();
    fileserver.connection_storage  = create_connection_storage(MAX_CONNECTION_COUNT);
    fileserver.receive_data_buffer = malloc(MAX_PACKET_SIZE * 24);

    fileserver.socket = socket_create_tcp();
    socket_set_nonblocking(fileserver.socket);

    fileserver.address = socket_create_inet_address("0.0.0.0", DEFAULT_LISTEN_PORT);
    socket_bind(fileserver.socket, fileserver.address);

    socket_address_to_string(&fileserver.address, fileserver.address_string, INET_STRADDR_LENGTH);

    socket_listen(fileserver.socket);

    return fileserver;
}

void
fileserver_destroy(fileserver* fileserver)
{
    file_io_destroy(&fileserver->io);
    destroy_connection_storage(&fileserver->connection_storage);
    free(fileserver->receive_data_buffer);
    socket_close(fileserver->socket);
}

int 
main(int argc, char* argv[]) 
{
    UNUSED(argc);
    UNUSED(argv);

    console_init();
    socket_initialize();

    measure_time measure;
    measure_initialize(&measure);

    fileserver fileserver = fileserver_create();

    int running = 1;
    while(running && !platform_quit)
    {
	    fileserver_tick(&fileserver);

        wait_for_target_ups(&measure, 1.0 / TARGET_UPS);

	    timer_end_frame(&fileserver.timer);
    }

    fileserver_destroy(&fileserver);

    socket_cleanup();

    printf("Server terminated gracefully.\n");
}

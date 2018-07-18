// fileserver.c

#include "platform.h"
#include "protocol.h"

#include "string_sanitize.c"
#include "connection.c"

#define TARGET_UPS 1000

typedef struct connection_statistics {
	int connections;
	int rejected_connections;
	int disconnections;
    int sent_bytes;
    int recv_bytes;
} connection_statistics;

float
print_server_info(char* listen_address, connection_storage* connection_storage,
                  timer* timer, connection_statistics* statistics, float last_print_time)
{
	if(last_print_time <= 0.0f || (timer->elapsed_seconds - last_print_time) > 1.0f)
    {
	    console_clear();

        printf("%-20s: %s\n", "Listen address", listen_address);

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

	connection->transfer_in_progress = 1;
	connection->transfer.file_size	 = upload_begin->file_size;
	connection->transfer.chunk_count = upload_begin->chunk_count;

    char* packet_file_name = (char*)(packet_body + sizeof(packet_file_upload_begin));
    santitize_file_name(packet_file_name, upload_begin->file_name_length, connection->transfer.file_name, MAX_FILE_NAME);

	connection->transfer.received_bytes	 = 0;
	connection->transfer.chunk_completed = 0;
}

void 
fileserver_receive_packet_body(connection* connection, int packet_type, char* packet_body, int body_length)
{
    UNUSED(packet_body);
    UNUSED(body_length);

    switch(packet_type)
    {
    case PACKET_DISCONNECT: 
        connection->pending_disconnect = 1;
        break;
    case PACKET_FILE_UPLOAD_BEGIN:
	    fileserver_upload_begin(connection, packet_body, body_length);
        break;
    case PACKET_FILE_UPLOAD_CHUNK: break;
    case PACKET_FILE_UPLOAD_FINAL: break;
    default: 
        connection_protocol_error(connection);
        break;
    }
}

void
fileserver_receive_packets(connection* connection, char* data, int length)
{
    int remaining_bytes = length;
    while(remaining_bytes > 0)
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

		selectable_set_set_read(selectable, connection->socket);
        
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
wait_for_target_ups(measure_time* measure, double target_delta)
{
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

int 
main(int argc, char* argv[]) 
{
    UNUSED(argc);
    UNUSED(argv);

    console_init();

    timer timer;
    timer_initialize(&timer);

    measure_time measure;
    measure_initialize(&measure);

    connection_statistics statistics;
    memset(&statistics, 0, sizeof(statistics));

    socket_initialize();

    socket_handle server_socket = socket_create_tcp();
    socket_set_nonblocking(server_socket);
    
    socket_address server_address = socket_create_inet_address("0.0.0.0", DEFAULT_LISTEN_PORT);

    socket_bind(server_socket, server_address);

    char server_address_string[INET_STRADDR_LENGTH];
    socket_address_to_string(&server_address, server_address_string, INET_STRADDR_LENGTH);

    socket_listen(server_socket);

    selectable_set selectable = selectable_set_create();

    connection_storage connection_storage = create_connection_storage(MAX_CONNECTION_COUNT);

    char* receive_data_buffer = malloc(MAX_PACKET_SIZE);

    float last_print_time = 0.0f;

    double target_frame_delta = 1.0 / TARGET_UPS;

    int running = 1;
    while(running && !platform_quit)
    {
	    last_print_time = print_server_info(server_address_string, &connection_storage, &timer,
	                                        &statistics, last_print_time);
	    
        selectable_set_clear(&selectable);

	    int highest_handle = process_connection_connections(&connection_storage, &selectable,
	                                                        &statistics);

	    int selected = selectable_set_select_noblock(&selectable, highest_handle);
        if(selected > 0)
        {
            // NOTE: Process already connected connections before accepting new connections.
            process_connection_network_io(&connection_storage, &selectable, receive_data_buffer,
                                          MAX_PACKET_SIZE, &statistics);
        }

        selectable_set_clear(&selectable);
	    selectable_set_set_read(&selectable, server_socket);

        highest_handle = server_socket + 1;
        selected = selectable_set_select_noblock(&selectable, highest_handle);

	    if(selected > 0 && selectable_set_can_read(&selectable, server_socket))
	    {
		    accept_incoming_connections(selected, server_socket, &connection_storage, &statistics);
	    }

        measure_tick(&measure);

        wait_for_target_ups(&measure, target_frame_delta);

	    timer_end_frame(&timer);
    }

    free(receive_data_buffer);

    destroy_connection_storage(&connection_storage);

    socket_close(server_socket);

    socket_cleanup();

    printf("Server terminated gracefully.\n");
}

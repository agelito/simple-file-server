// fileserver.c

#include "platform.h"
#include "protocol.h"

#include "connection.c"

#define MAX_CONNECTION_COUNT 1024

void
print_server_info(int print_frequency, char* listen_address, connection_storage* connection_storage, timer* timer)
{
    if(timer->frame_counter == 0 || timer->frame_counter % print_frequency == 0)
    {
	    console_clear();

        printf("%-20s: %s\n", "Listen address", listen_address);

        float average = (float)1 / print_frequency;
        printf("%-20s: %.02fms %.02fups %.02fmcy\n", "Performance", 
               timer->delta_milliseconds * average, 
               timer->frames_per_second * average, 
               timer->megacycles_per_frame * average);
    
        timer_reset_accumulators(timer);

        printf("%-20s: %d / %d\n", "Connections", connection_storage->count,
               connection_storage->capacity);
    }
}

void 
connection_receive_packet_body(connection* connection, int packet_type, char* packet_body, int body_length)
{
    UNUSED(packet_body);
    UNUSED(body_length);

    switch(packet_type)
    {
    case PACKET_DISCONNECT: break;
    case PACKET_FILE_UPLOAD_BEGIN: break;
    case PACKET_FILE_UPLOAD_CHUNK: break;
    case PACKET_FILE_UPLOAD_FINAL: break;
    default: 
        connection->pending_disconnect = 1;
        break;
    }
}

void
connection_receive_packet(connection* connection, char* data, int length)
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
        connection_receive_packet_body(connection, header->packet_type, packet_data_body,
                                   header->packet_size - sizeof(packet_header));

        data = (data + header->packet_size);
    }
}

void
accept_incoming_connections(int count, socket_handle socket, connection_storage* connection_storage)
{
	int max_incoming_per_frame = 32;
	if(count > max_incoming_per_frame)
		count = max_incoming_per_frame;
	
	while(0 < count--)
	{
		socket_address accepted_address;
		socket_handle accepted = socket_accept(socket, &accepted_address);
		if(accepted == 0) break;

		connection* new_connection  = create_new_connection(connection_storage);
		if(new_connection != 0)
		{
			socket_set_nonblocking(accepted);

			new_connection->socket  = accepted;
			new_connection->address = accepted_address;
            
			socket_address_to_string(&accepted_address, new_connection->address_string,
			                         INET_STRADDR_LENGTH);

			printf("Connection created: %s\n", new_connection->address_string);
		}
		else
		{
			printf("Can't create connection. Too many connections.\n");
			socket_close(accepted);
		}
	}
}

int
process_connection_connections(connection_storage* connection_storage, selectable_set* selectable)
{
	int highest_handle = 0;
	int connection_index;
	for(connection_index = 0; connection_index < connection_storage->count; ++connection_index)
	{
		connection* connection = (connection_storage->connections + connection_index);

		if(connection->pending_disconnect && connection->send_bytes == 0)
		{
			connection_disconnect(connection);
		}
            
		if(!connection->socket)
		{
			remove_connection_index(connection_storage, connection_index--);
			continue;
		}

		selectable_set_set_read(selectable, connection->socket);

		if(connection->socket > highest_handle)
			highest_handle = connection->socket;
	}

	return highest_handle;
}

void
process_connection_network_io(connection_storage* connection_storage, selectable_set* selectable,
                          char* io_buffer, int io_buffer_size)
{
	int connection_index;
	for(connection_index = 0; connection_index < connection_storage->count; ++connection_index)
	{
		connection* connection = (connection_storage->connections + connection_index);
		if(selectable_set_can_write(selectable, connection->socket))
		{
			int sent_bytes = 0;
			while(sent_bytes < connection->send_bytes)
			{
				int remaining = connection->send_bytes - sent_bytes;
				int send_result =
					socket_send(connection->socket, connection->send_data + sent_bytes, remaining);
				if(send_result > 0)
				{
					sent_bytes += send_result;
				}
				else
				{
					connection_disconnect(connection);
					break;
				}
			}

			connection->send_bytes = 0;
		}

		if(selectable_set_can_read(selectable, connection->socket))
		{
			int recv_result = socket_recv(connection->socket, io_buffer, io_buffer_size);
			if(recv_result > 0)
			{
				connection_receive_packet(connection, io_buffer, recv_result);
			}
			else
			{
				connection_disconnect(connection);
			}
		}
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

    socket_initialize();

    socket_handle server_socket = socket_create_tcp();

    socket_address server_address = socket_create_inet_address("0.0.0.0", DEFAULT_LISTEN_PORT);

    socket_bind(server_socket, server_address);

    char server_address_string[INET_STRADDR_LENGTH];
    socket_address_to_string(&server_address, server_address_string, INET_STRADDR_LENGTH);

    socket_listen(server_socket);

    selectable_set selectable = selectable_set_create();

    connection_storage connection_storage = create_connection_storage(MAX_CONNECTION_COUNT);

    char* receive_data_buffer = malloc(MAX_PACKET_SIZE);

    int running = 1;
    while(running && !platform_quit)
    {
	    print_server_info(100, server_address_string, &connection_storage, &timer);
	    
	    selectable_set_clear(&selectable);
	    selectable_set_set_read(&selectable, server_socket);

	    int highest_handle = process_connection_connections(&connection_storage, &selectable);
	    if(server_socket > highest_handle)
		    highest_handle = server_socket;

	    int selected = selectable_set_select(&selectable, highest_handle, 10);

	    // NOTE: Process already connected connections before accepting new connections.
	    process_connection_network_io(&connection_storage, &selectable, receive_data_buffer, MAX_PACKET_SIZE);

	    if(selected > 0 && selectable_set_can_read(&selectable, server_socket))
	    {
		    accept_incoming_connections(selected, server_socket, &connection_storage);
	    }

	    timer_end_frame(&timer);
    }

    free(receive_data_buffer);

    destroy_connection_storage(&connection_storage);

    socket_close(server_socket);

    socket_shutdown();

    printf("Server terminated gracefully.\n");
}

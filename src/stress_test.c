// stress_test.c

#include "platform.h"
#include "protocol.h"

#include "connection.c"

#define TEST_ITERATIONS         1024
#define TEST_CONNECTION_COUNT   256

void
create_connections(int count, int iterations)
{
    connection_storage connections = create_connection_storage(count);

	int iteration;
	for(iteration = 0; iteration < iterations; ++iteration)
	{
		console_clear();

		printf("Iteration %4d/%4d\n", iteration+1, iterations);

        socket_address server_address = socket_create_inet_address("127.0.0.1", DEFAULT_LISTEN_PORT);
	
		int i;

        printf("Creating %d connections...\n", count);
		for(i = 0; i < count; ++i)
		{
			socket_handle socket = socket_create_tcp();
			socket_set_blocking(socket);

            connection* connection = create_new_connection(&connections);
            connection->socket = socket;
            connection->address = server_address;
            connection->socket_initialized = 1;

            socket_address_to_string(&server_address, connection->address_string, INET_STRADDR_LENGTH);
		}

		printf("Connecting %d...\n", count);
		for(i = 0; i < connections.count; ++i)
		{
            connection* connection = (connections.connections + i);
			socket_connect(connection->socket, &server_address);
		}

        printf("Sending disconnect packets...\n");
        for(i = 0; i < connections.count; ++i)
        {
            connection* connection = (connections.connections + i);
            connection_push_empty_packet(connection, PACKET_DISCONNECT);
        }

        int sent_bytes = 0;
        for(i = 0; i < connections.count; ++i)
        {
            connection* connection = (connections.connections + i);
            if(connection->send_bytes > 0)
            {
                sent_bytes += connection_send_network_data(connection);
            }
        }

		printf("Closing %d connections.\n", count);
		for(i = 0; i < connections.count; ++i)
		{
            connection* connection = (connections.connections + i);
            remove_connection_index(&connections, i--);
            connection_disconnect(connection);
		}

        thread_sleep(50);
	}
    
    destroy_connection_storage(&connections);
}

int
main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

	socket_initialize();

	create_connections(TEST_CONNECTION_COUNT, TEST_ITERATIONS);

	socket_shutdown();

	return 0;
}

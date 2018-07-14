// stress_test.c

#include "platform.h"
#include "protocol.h"

#define TEST_ITERATIONS         1024
#define TEST_CONNECTION_COUNT	256

void
create_connections(int count, int iterations)
{
	socket_handle* sockets = (socket_handle*)malloc(sizeof(socket_handle) * count);

	int iteration;
	for(iteration = 0; iteration < iterations; ++iteration)
	{
		console_clear();

		printf("Iteration %4d/%4d\n", iteration+1, iterations);
		printf("Creating %d sockets...\n", count);
	
		int i;
		for(i = 0; i < count; ++i)
		{
			socket_handle socket = socket_create_tcp();
			socket_set_blocking(socket);
			*(sockets + i) = socket;
		}

		socket_address server_address = socket_create_inet_address("127.0.0.1", DEFAULT_LISTEN_PORT);

		printf("Connecting %d sockets...\n", count);
	
		for(i = 0; i < count; ++i)
		{
			socket_handle socket = *(sockets + i);

			socket_connect(socket, &server_address);
		}

		printf("Closing %d sockets.\n", count);

		for(i = 0; i < count; ++i)
		{
			socket_handle socket = *(sockets + i);
			socket_close(socket);
		}
	}
}

int
main(int argc, char* argv[])
{
	socket_initialize();

	create_connections(TEST_CONNECTION_COUNT, TEST_ITERATIONS);

	socket_shutdown();

	return 0;
}

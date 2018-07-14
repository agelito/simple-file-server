// server_locator.c

#include "platform.h"
#include "protocol.h"

#define RECEIVE_BUFFER_LENGTH 4096
#define SENDING_BUFFER_LENGTH 1024

#define SEND_QUEUE_CAPACITY 128

int 
main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    socket_initialize();

    socket_handle listen_socket = socket_create_udp();

    socket_enable_broadcast(listen_socket);

    socket_address listen_address = socket_create_inet_address("0.0.0.0", DEFAULT_RECEIVE_PORT);

    socket_bind(listen_socket, listen_address);

    char* receive_buffer = (char*)malloc(RECEIVE_BUFFER_LENGTH);

    socket_address receive_address;
    memset(&receive_address, 0, sizeof(socket_address));

    socket_handle sending_socket = socket_create_udp();

    char* sending_buffer = (char*)malloc(SENDING_BUFFER_LENGTH);
    memset(sending_buffer, 0, SENDING_BUFFER_LENGTH);

    socket_address sending_address = socket_create_inet_address("0.0.0.0", DEFAULT_SENDING_PORT);

    int send_queue_count = 0;
    int* send_to_address = (int*)malloc(sizeof(int) * SEND_QUEUE_CAPACITY);

    char listen_address_string[INET_STRADDR_LENGTH];
    socket_address_to_string(&listen_address, listen_address_string, INET_STRADDR_LENGTH);

    printf("Locator server listening: %s\n", listen_address_string);

    int listening = 1;
    while(listening)
    {
	    int receive_result = socket_recvfrom(listen_socket, receive_buffer, RECEIVE_BUFFER_LENGTH, 
	                                         &receive_address);
	    listening = (receive_result != 0);
         
        if(receive_result > 0 && send_queue_count < SEND_QUEUE_CAPACITY)
        {
            *(send_to_address + send_queue_count++) = 
	            socket_inet_address_get_addr(&receive_address);
        }

        for(int i = 0; i < send_queue_count; ++i)
        {
            socket_inet_address_set_addr(&sending_address, *(send_to_address + i));

            char address[INET_STRADDR_LENGTH];
            socket_address_to_string(&sending_address, address, INET_STRADDR_LENGTH);

            printf("%s -> %s\n", listen_address_string, address);

            socket_sendto(sending_socket, sending_buffer, 1, &sending_address);
        }
        send_queue_count = 0;

        thread_sleep(10);
    }

    free(receive_buffer);
    free(sending_buffer);
    free(send_to_address);

    socket_close(listen_socket);

    socket_shutdown();

    return 0;
}

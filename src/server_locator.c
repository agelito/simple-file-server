// server_locator.c

#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "common.c"
#include "socket_wsa.c"
#include "protocol.h"

#define RECEIVE_BUFFER_LENGTH 4096
#define SENDING_BUFFER_LENGTH 1024

#define SEND_QUEUE_CAPACITY 128

int 
main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    CHECK_WSA_ERROR();

    SOCKET listen_socket = socket_create_udp();

    socket_enable_broadcast(listen_socket);

    struct sockaddr_in listen_address;
    memset(&listen_address, 0, sizeof(struct sockaddr_in));
    listen_address.sin_family      = AF_INET;
    listen_address.sin_port        = htons(DEFAULT_RECEIVE_PORT);
    listen_address.sin_addr.s_addr = 0;

    bind(listen_socket, (struct sockaddr*)&listen_address, sizeof(struct sockaddr_in));
    CHECK_WSA_ERROR();

    char* receive_buffer = (char*)malloc(RECEIVE_BUFFER_LENGTH);

    struct sockaddr_in receive_address;
    memset(&receive_address, 0, sizeof(struct sockaddr_in));
    int receive_address_length = sizeof(struct sockaddr_in);

    SOCKET sending_socket = socket_create_udp();

    char* sending_buffer = (char*)malloc(SENDING_BUFFER_LENGTH);
    memset(sending_buffer, 0, SENDING_BUFFER_LENGTH);

    struct sockaddr_in sending_address;
    memset(&sending_address, 0, sizeof(struct sockaddr_in));
    sending_address.sin_family      = AF_INET;
    sending_address.sin_port        = htons(DEFAULT_SENDING_PORT);
    sending_address.sin_addr.s_addr = 0;

    int send_queue_count = 0;
    int* send_to_address = (int*)malloc(sizeof(int) * SEND_QUEUE_CAPACITY);

    wchar_t listen_address_string[INET_STRADDR_LENGTH];
    socket_address_to_string(&listen_address, listen_address_string, INET_STRADDR_LENGTH);

    int listening = 1;
    while(listening)
    {
        int receive_result = recvfrom(listen_socket, receive_buffer, RECEIVE_BUFFER_LENGTH, 
                                      0, (struct sockaddr*)&receive_address, &receive_address_length);
        listening = (receive_result != 0);
        CHECK_WSA_ERROR();
        
        if(receive_result > 0 && send_queue_count < SEND_QUEUE_CAPACITY)
        {
            *(send_to_address + send_queue_count++) = 
                receive_address.sin_addr.s_addr;
        }

        for(int i = 0; i < send_queue_count; ++i)
        {
            sending_address.sin_addr.s_addr = *(send_to_address + i);

            wchar_t address[INET_STRADDR_LENGTH];
            socket_address_to_string(&sending_address, address, INET_STRADDR_LENGTH);

            printf("%S -> %S\n", &listen_address_string, address);

            sendto(sending_socket, sending_buffer, 1, 0,
                   (struct sockaddr*)&sending_address, 
                   sizeof(struct sockaddr_in));
            CHECK_WSA_ERROR();
        }
        send_queue_count = 0;

        Sleep(10);
    }

    free(receive_buffer);
    free(sending_buffer);
    free(send_to_address);

    closesocket(listen_socket);

    WSACleanup();

    return 0;
}

// fileserver.c

#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "protocol.h"

#include "common.c"
#include "console_utils.c"
#include "socket_wsa.c"
#include "client.c"
#include "timer.c"

#define MILLISECONDS_TO_MICROSECONDS 1000

#define MAX_CLIENT_COUNT 1024

void
print_server_info(HANDLE console, int print_frequency, wchar_t* listen_address, client_storage* client_storage, timer* timer)
{
    if(timer->frame_counter == 0 || timer->frame_counter % print_frequency == 0)
    {
        clear_console(console);

        printf("%-20S: %S\n", L"Listen address", listen_address);

        float average = (float)1 / print_frequency;
        printf("%-20S: %.02fms %.02ffps %.02fmcy\n", L"Performance", 
               timer->delta_milliseconds * average, 
               timer->frames_per_second * average, 
               timer->megacycles_per_frame * average);
    
        timer_reset_accumulators(timer);

        printf("%-20S: %d / %d\n", L"Connected clients", client_storage->count, client_storage->capacity);
    }
}

void 
client_receive_packet_body(client* client, int packet_type, char* packet_body, int body_length)
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
        client->pending_disconnect = 1;
        break;
    }
}

void
client_receive_packet(client* client, char* data, int length)
{
    int remaining_bytes = length;
    while(remaining_bytes > 0)
    {
        packet_header* header = (packet_header*)data;
        if(header->packet_size > remaining_bytes || header->packet_type == PACKET_INVALID_PROTOCOL)
        {
            client->pending_disconnect = 1;
            break;
        }

        char* packet_data_body = (char*)header + sizeof(packet_header);
        client_receive_packet_body(client, header->packet_type, packet_data_body, header->packet_size - sizeof(packet_header));

        data = (data + header->packet_size);
    }
}

int 
main(int argc, char* argv[]) 
{
    UNUSED(argc);
    UNUSED(argv);

    console_utils_init();

    timer timer;
    timer_initialize(&timer);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    CHECK_WSA_ERROR();

    SOCKET server_socket = socket_create_tcp();

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(struct sockaddr_in));
    server_address.sin_family      = AF_INET;
    server_address.sin_port        = htons(DEFAULT_LISTEN_PORT);
    server_address.sin_addr.s_addr = 0;

    bind(server_socket, (struct sockaddr*)&server_address, sizeof(struct sockaddr_in));
    CHECK_WSA_ERROR();

    wchar_t server_address_string[INET_STRADDR_LENGTH];
    socket_address_to_string(&server_address, server_address_string, INET_STRADDR_LENGTH);

    listen(server_socket, SOMAXCONN);
    CHECK_WSA_ERROR();

    FD_SET read_set;
    FD_SET write_set;

    struct timeval select_timeout;
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 10 * MILLISECONDS_TO_MICROSECONDS;

    client_storage client_storage = create_client_storage(MAX_CLIENT_COUNT);

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    char* receive_data_buffer = malloc(MAX_PACKET_SIZE);

    int running = 1;
    while(running && !console_indicate_quit)
    {
        print_server_info(console, 10, server_address_string, &client_storage, &timer);

        FD_ZERO(&read_set);
        FD_SET(server_socket, &read_set);

        FD_ZERO(&write_set);

        int client_index;
        for(client_index = 0; client_index < client_storage.count; ++client_index)
        {
            client* client = (client_storage.clients + client_index);

            if(client->pending_disconnect && client->send_bytes == 0)
            {
                client_disconnect(client);
            }
            
            if(!client->socket)
            {
                remove_client_index(&client_storage, client_index--);
                continue;
            }

            FD_SET(client->socket, &read_set);
        }

        int selected = select(0, &read_set, &write_set, 0, &select_timeout);
        if(selected > 0 && FD_ISSET(server_socket, &read_set))
        {
            struct sockaddr_in accepted_address;
            int accepted_address_length = sizeof(struct sockaddr_in);

            SOCKET accepted = accept(server_socket, (struct sockaddr*)&accepted_address, &accepted_address_length);

            client* new_client  = create_new_client(&client_storage);
            if(new_client != 0)
            {
                socket_set_nonblocking(accepted);

                new_client->socket  = accepted;
                new_client->address = accepted_address;
            
                socket_address_to_string(&accepted_address, new_client->address_string, INET_STRADDR_LENGTH);

                printf("Client connected: %S\n", new_client->address_string);
            }
            else
            {
                printf("Can't connect Client. Too many connections.\n");
                closesocket(accepted);
            }
        }

        for(client_index = 0; client_index < client_storage.count; ++client_index)
        {
            client* client = (client_storage.clients + client_index);
            if(FD_ISSET(client->socket, &write_set))
            {
                int sent_bytes = 0;
                while(sent_bytes < client->send_bytes)
                {
                    int remaining = client->send_bytes - sent_bytes;
                    int send_result = send(client->socket, client->send_data + sent_bytes, remaining, 0);
                    if(send_result == SOCKET_ERROR)
                    {
                        // TODO: More robust error handling for sending data.
                        CHECK_WSA_ERROR();
                    }
                    else if(send_result > 0)
                    {
                        sent_bytes += send_result;
                    }
                    else
                    {
                        client_disconnect(client);
                        break;
                    }
                }

                client->send_bytes = 0;
            }

            if(FD_ISSET(client->socket, &read_set))
            {
                int recv_result = recv(client->socket, receive_data_buffer, MAX_PACKET_SIZE, 0);
                if(recv_result == SOCKET_ERROR)
                {
                    // TODO: More robust error handling for client data. Currently clients could
                    // send malformed data packets to make the server shut down.
                    CHECK_WSA_ERROR();
                }
                else if(recv_result > 0)
                {
                    client_receive_packet(client, receive_data_buffer, recv_result);
                }
                else
                {
                    client_disconnect(client);
                }
            }
        }

        timer_end_frame(&timer);
    }

    free(receive_data_buffer);

    destroy_client_storage(&client_storage);

    closesocket(server_socket);

    WSACleanup();

    printf("Server terminated gracefully.\n");
}

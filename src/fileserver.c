// fileserver.c

#include "platform.h"
#include "protocol.h"

#include "client.c"

#define MAX_CLIENT_COUNT 1024

int platform_quit = 0;

void
print_server_info(int print_frequency, char* listen_address, client_storage* client_storage, timer* timer)
{
    if(timer->frame_counter == 0 || timer->frame_counter % print_frequency == 0)
    {
	    console_clear();

        printf("%-20s: %s\n", "Listen address", listen_address);

        float average = (float)1 / print_frequency;
        printf("%-20s: %.02fms %.02ffps %.02fmcy\n", "Performance", 
               timer->delta_milliseconds * average, 
               timer->frames_per_second * average, 
               timer->megacycles_per_frame * average);
    
        timer_reset_accumulators(timer);

        printf("%-20s: %d / %d\n", "Connected clients", client_storage->count, client_storage->capacity);
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

    client_storage client_storage = create_client_storage(MAX_CLIENT_COUNT);

    char* receive_data_buffer = malloc(MAX_PACKET_SIZE);

    int running = 1;
    while(running && !platform_quit)
    {
        print_server_info(100, server_address_string, &client_storage, &timer);

        selectable_set_clear(&selectable);
        selectable_set_set_read(&selectable, server_socket);

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

            selectable_set_set_read(&selectable, client->socket);
        }

        int selected = selectable_set_select(&selectable, 10);
        if(selected > 0 && selectable_set_can_read(&selectable, server_socket))
        {
            socket_address accepted_address;
            socket_handle accepted = socket_accept(server_socket, &accepted_address);

            client* new_client  = create_new_client(&client_storage);
            if(new_client != 0)
            {
                socket_set_nonblocking(accepted);

                new_client->socket  = accepted;
                new_client->address = accepted_address;
            
                socket_address_to_string(&accepted_address, new_client->address_string, INET_STRADDR_LENGTH);

                printf("Client connected: %s\n", new_client->address_string);
            }
            else
            {
                printf("Can't connect Client. Too many connections.\n");
                socket_close(accepted);
            }
        }

        for(client_index = 0; client_index < client_storage.count; ++client_index)
        {
            client* client = (client_storage.clients + client_index);
            if(selectable_set_can_write(&selectable, client->socket))
            {
                int sent_bytes = 0;
                while(sent_bytes < client->send_bytes)
                {
                    int remaining = client->send_bytes - sent_bytes;
                    int send_result =
	                    socket_send(client->socket, client->send_data + sent_bytes, remaining);
                    if(send_result > 0)
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

            if(selectable_set_can_read(&selectable, client->socket))
            {
                int recv_result = socket_recv(client->socket, receive_data_buffer, MAX_PACKET_SIZE);
                if(recv_result > 0)
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

    socket_close(server_socket);

    socket_shutdown();

    printf("Server terminated gracefully.\n");
}

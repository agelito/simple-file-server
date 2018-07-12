// client.c

typedef struct client {
    socket_handle      socket;
    socket_address     address;
	char               address_string[INET_STRADDR_LENGTH];
    int                pending_disconnect;
    int                send_bytes;
    char*              send_data;
} client;

typedef struct client_storage {
    int count;
    int capacity;
    client* clients;
} client_storage;

client_storage 
create_client_storage(int capacity);

void
destroy_client_storage(client_storage* client_storage);

client* 
create_new_client(client_storage* client_storage);

void 
remove_client(client_storage* client_storage, client* client);

client_storage 
create_client_storage(int capacity)
{
    client_storage client_storage;
    client_storage.count    = 0;
    client_storage.capacity = capacity;
    client_storage.clients  = (client*)malloc(sizeof(client) * capacity);

    return client_storage;
}

void
destroy_client_storage(client_storage* client_storage)
{
    int i;
    for(i = 0; i < client_storage->count; ++i)
    {
        client* client = (client_storage->clients + i);
        if(client->socket)
            socket_close(client->socket);
    }

    client_storage->count = 0;
    client_storage->capacity = 0;

    free(client_storage->clients);
    client_storage->clients = 0;
}

client* 
create_new_client(client_storage* client_storage)
{
    client* new_client = 0;
    if(client_storage->count < client_storage->capacity)
    {
        new_client = (client_storage->clients + client_storage->count++);
        new_client->socket             = 0;
        new_client->pending_disconnect = 0;
    }
    return new_client;
}

void
client_disconnect(client* client)
{
    if(client->socket)
    {
        printf("Disconnect client: %s\n", client->address_string);
        socket_close(client->socket);
        client->socket = 0;
    }
}

void 
remove_client_index(client_storage* client_storage, int index)
{
    client* client = (client_storage->clients + index);
    if(client->socket)
    {
        socket_close(client->socket);
        client->socket = 0;
    }

    int new_count = client_storage->count - 1;
    if(new_count > 0)
    {
        *(client_storage->clients + index) = *(client_storage->clients + new_count);
    }

    client_storage->count = new_count;
}

void
client_push_packet(client* client, int packet_type, char* packet_data, int packet_length)
{
    char* send_data = client->send_data;

    int full_packet_length = sizeof(packet_header) + packet_length;
    int remaining_bytes = MAX_PACKET_SIZE - client->send_bytes;
    if(full_packet_length < remaining_bytes)
    {
        packet_header* header = (packet_header*)(send_data + client->send_bytes);
        header->packet_type = packet_type;
        header->packet_size = full_packet_length;

        send_data += sizeof(packet_header);

        if(packet_length > 0)
        {
            memcpy(send_data, packet_data, packet_length);
            client->send_bytes += full_packet_length;
        }
    }
}

void
client_push_empty_packet(client* client, int packet_type)
{
    client_push_packet(client, packet_type, 0, 0);
}


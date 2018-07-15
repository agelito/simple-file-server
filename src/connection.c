// connection.c

typedef struct connection {
    socket_handle      socket;
    socket_address     address;
	char               address_string[INET_STRADDR_LENGTH];
    int                pending_disconnect;
    int                send_bytes;
    char*              send_data;
} connection;

typedef struct connection_storage {
    int count;
    int capacity;
    connection* connections;
} connection_storage;

connection_storage 
create_connection_storage(int capacity);

void
destroy_connection_storage(connection_storage* connection_storage);

connection* 
create_new_connection(connection_storage* connection_storage);

void 
remove_connection(connection_storage* connection_storage, connection* connection);

connection_storage 
create_connection_storage(int capacity)
{
    connection_storage connection_storage;
    connection_storage.count    = 0;
    connection_storage.capacity = capacity;
    connection_storage.connections  = (connection*)malloc(sizeof(connection) * capacity);

    return connection_storage;
}

void
destroy_connection_storage(connection_storage* connection_storage)
{
    int i;
    for(i = 0; i < connection_storage->count; ++i)
    {
        connection* connection = (connection_storage->connections + i);
        if(connection->socket)
            socket_close(connection->socket);
    }

    connection_storage->count = 0;
    connection_storage->capacity = 0;

    free(connection_storage->connections);
    connection_storage->connections = 0;
}

connection* 
create_new_connection(connection_storage* connection_storage)
{
    connection* new_connection = 0;
    if(connection_storage->count < connection_storage->capacity)
    {
        new_connection = (connection_storage->connections + connection_storage->count++);
        new_connection->socket             = 0;
        new_connection->pending_disconnect = 0;
    }
    return new_connection;
}

void
connection_disconnect(connection* connection)
{
    if(connection->socket)
    {
        socket_close(connection->socket);
        connection->socket = 0;
    }
}

void 
remove_connection_index(connection_storage* connection_storage, int index)
{
    connection* connection = (connection_storage->connections + index);
    if(connection->socket)
    {
        socket_close(connection->socket);
        connection->socket = 0;
    }

    int new_count = connection_storage->count - 1;
    if(new_count > 0)
    {
        *(connection_storage->connections + index) = *(connection_storage->connections + new_count);
    }

    connection_storage->count = new_count;
}

void
connection_push_packet(connection* connection, int packet_type, char* packet_data, int packet_length)
{
    char* send_data = connection->send_data;

    int full_packet_length = sizeof(packet_header) + packet_length;
    int remaining_bytes = MAX_PACKET_SIZE - connection->send_bytes;
    if(full_packet_length < remaining_bytes)
    {
        packet_header* header = (packet_header*)(send_data + connection->send_bytes);
        header->packet_type = packet_type;
        header->packet_size = full_packet_length;

        send_data += sizeof(packet_header);

        if(packet_length > 0)
        {
            memcpy(send_data, packet_data, packet_length);
            connection->send_bytes += full_packet_length;
        }
    }
}

void
connection_push_empty_packet(connection* connection, int packet_type)
{
    connection_push_packet(connection, packet_type, 0, 0);
}


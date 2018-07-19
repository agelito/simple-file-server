// connection.c

typedef struct connection_file_transfer
{
	int	 file_size;
	int	 received_bytes;
	int	 chunk_count;
	int	 chunk_completed;
	char file_name_final[MAX_FILE_NAME];
    file_io_file download;
} connection_file_transfer;

typedef struct connection
{
    socket_handle			 socket;
    socket_address			 address;
	char					 address_string[INET_STRADDR_LENGTH];
    int                      socket_initialized;
    int						 pending_disconnect;
    int						 send_bytes;
    char*					 send_data;
	int						 transfer_in_progress;
	connection_file_transfer transfer;
} connection;

typedef struct connection_storage
{
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

connection_storage 
create_connection_storage(int capacity)
{
    connection_storage connection_storage;
    connection_storage.count    = 0;
    connection_storage.capacity = capacity;
    connection_storage.connections  = (connection*)malloc(sizeof(connection) * capacity);
    memset(connection_storage.connections, 0, sizeof(connection) * capacity);

    int i;
    for(i = 0; i < connection_storage.capacity; ++i)
    {
        connection* connection = (connection_storage.connections + i);

        // TODO: The size of send buffer is arbritrary, need to tweak
        // depending on desired memory footprint.
        connection->send_data = malloc(MAX_PACKET_SIZE * 4);
        connection->send_bytes = 0;
    }

    return connection_storage;
}

void
destroy_connection_storage(connection_storage* connection_storage)
{
    int i;
    for(i = 0; i < connection_storage->capacity; ++i)
    {
        connection* connection = (connection_storage->connections + i);
        if(connection->socket)
            socket_close(connection->socket);

        if(connection->send_data)
            free(connection->send_data);
        
        connection->send_data = 0;
        connection->send_bytes = 0;
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

        new_connection->socket				 = 0;
        new_connection->socket_initialized	 = 0;
        new_connection->pending_disconnect	 = 0;
        new_connection->send_bytes			 = 0;
        new_connection->transfer_in_progress = 0;
        
        memset(&new_connection->transfer, 0, sizeof(connection_file_transfer));
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
    connection* connection_at_index = (connection_storage->connections + index);
    if(connection_at_index->socket)
    {
        socket_close(connection_at_index->socket);
        connection_at_index->socket = 0;
    }

    connection_at_index->send_bytes = 0;

    int new_count = connection_storage->count - 1;
    if(new_count > 0)
    {
        connection removed_connection = *(connection_storage->connections + index);
        *(connection_storage->connections + index) = *(connection_storage->connections + new_count);
        *(connection_storage->connections + new_count) = removed_connection;
    }

    connection_storage->count = new_count;
}

int
connection_push_packet(connection* connection, char packet_type, char* packet_data, short packet_length)
{
    char* send_data = connection->send_data;

    short full_packet_length = sizeof(packet_header) + packet_length;
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
        }

        connection->send_bytes += full_packet_length;

        return 1;
    }

    return 0;
}

int
connection_push_empty_packet(connection* connection, char packet_type)
{
    return connection_push_packet(connection, packet_type, 0, 0);
}

void 
connection_protocol_error(connection* connection)
{
    connection_push_empty_packet(connection, PACKET_INVALID_PROTOCOL);
    connection->pending_disconnect = 1;
}

int
connection_send_network_data(connection* connection)
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
            sent_bytes = -1;
            break;
        }
    }

    connection->send_bytes = 0;
    return sent_bytes;
}

int
connection_recv_network_data(connection* connection, char* io_buffer, int io_buffer_size)
{
    int recv_result = socket_recv(connection->socket, io_buffer, io_buffer_size);
    if(recv_result == -1)
    {
        connection_disconnect(connection);
    }

    return recv_result;
}


// connection.c

typedef struct connection_file_transfer
{
	int64_t     file_size;
    int64_t     byte_count_recv;
	int64_t     byte_count_disk;
	char        file_name_final[MAX_FILE_NAME];
    int         io_buffer_size;
    int         io_buffer_capacity;
    char*       io_buffer;
    mapped_file download_file;
} connection_file_transfer;

typedef struct connection
{
    socket_handle			 socket;
    socket_address			 address;
	char					 address_string[INET_STRADDR_LENGTH];
    int                      socket_initialized;
    int                      request_disconnect;
    int						 pending_disconnect;
    int                      error;
    int						 send_data_count;
	int                      send_data_capacity;
    char*					 send_data;
    int                      recv_data_count;
    int                      recv_data_capacity;
    char*                    recv_data;
    int                      transfer_completed;
	int						 transfer_in_progress;
	connection_file_transfer transfer;
} connection;

typedef struct connection_storage
{
    int         count;
    int         capacity;
    connection* connections;
} connection_storage;

connection_storage 
create_connection_storage(int capacity);

void
destroy_connection_storage(connection_storage* connection_storage);

connection* 
create_new_connection(connection_storage* connection_storage);

void
connection_transfer_cancel(connection* connection);

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
        connection->send_data_count	   = 0;
        connection->send_data_capacity = (MAX_PACKET_SIZE * 16);
        connection->send_data		   = malloc(MAX_PACKET_SIZE * 16);
        connection->recv_data_count    = 0;
        connection->recv_data_capacity = (MAX_PACKET_SIZE * 16);
        connection->recv_data          = malloc(MAX_PACKET_SIZE * 16);
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
        
        connection->send_data		   = 0;
        connection->send_data_count	   = 0;
        connection->send_data_capacity = 0;

        if(connection->recv_data)
            free(connection->recv_data);

        connection->recv_data		   = 0;
        connection->recv_data_count	   = 0;
        connection->recv_data_capacity = 0;
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

        // NOTE: All of the connection fields isn't set to zero because the 
        // allocation of memory for the connection only happens once. Perhaps
        // this data pointers should be maintained somewhere else and the
        // socket just retrieve a reference to that location instead.

        new_connection->socket				 = 0;
        new_connection->socket_initialized	 = 0;
        new_connection->request_disconnect   = 0;
        new_connection->pending_disconnect	 = 0;
        new_connection->error                = 0;
        new_connection->send_data_count		 = 0;
        new_connection->recv_data_count      = 0;
        new_connection->transfer_in_progress = 0;
        
        memset(&new_connection->transfer, 0, sizeof(connection_file_transfer));
    }
    return new_connection;
}

void
connection_disconnect(connection* connection)
{
    connection_transfer_cancel(connection);

    if(connection->socket)
    {
        socket_close(connection->socket);
        connection->socket = 0;
    }
}

void
connection_error(connection* connection)
{
    connection->error = 1;
}

void
connection_transfer_free(connection_file_transfer* transfer)
{
    if(transfer->io_buffer)
    {
        free(transfer->io_buffer);
        transfer->io_buffer          = 0;
        transfer->io_buffer_size     = 0;
        transfer->io_buffer_capacity = 0;
    }

    if(transfer->download_file.file_path)
    {
        char file_path[MAX_FILE_NAME];
        strcpy(file_path, transfer->download_file.file_path);

        filesystem_destroy_mapped_file(&transfer->download_file);
        filesystem_delete_file(file_path);
    }

    transfer->byte_count_recv = 0;
    transfer->byte_count_disk = 0;
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

    connection_transfer_free(&connection_at_index->transfer);

    connection_at_index->transfer_in_progress = 0;

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
    char* send_data = (connection->send_data + connection->send_data_count);

    short header_length = sizeof(packet_header);
    short full_length = (header_length + packet_length);

    int remaining_bytes = (connection->send_data_capacity - connection->send_data_count);
    if(full_length < remaining_bytes)
    {
        packet_header* header = (packet_header*)send_data;
        header->packet_type = packet_type;
        header->packet_size = packet_length;

        send_data += sizeof(packet_header);

        if(packet_length > 0)
        {
            memcpy(send_data, packet_data, packet_length);
        }

        connection->send_data_count += header_length;
        connection->send_data_count += packet_length;

        return 1;
    }

    return 0;
}

int
connection_push_data_packet(connection* connection, char packet_type, char* packet, 
                            short packet_length, char* data, short data_length)
{
	char*	send_data			   = (connection->send_data + connection->send_data_count);
    short	header_length		   = sizeof(packet_header);
    short	packet_and_data_length = packet_length + data_length;
    int		remaining_bytes		   = (connection->send_data_capacity - connection->send_data_count);
    int     full_length            = header_length + packet_and_data_length;

    if(full_length < remaining_bytes)
    {
        packet_header* header = (packet_header*)send_data;
        header->packet_type = packet_type;
        header->packet_size = packet_and_data_length;

        send_data += sizeof(packet_header);

        if(packet_length > 0)
        {
            memcpy(send_data, packet, packet_length);
        }

        send_data += packet_length;

        if(data_length > 0)
        {
            memcpy(send_data, data, data_length);
        }

        connection->send_data_count += header_length;
        connection->send_data_count += packet_length;
        connection->send_data_count += data_length;

        return 1;
    }
    else
    {
        panic("Couldn't submit packet, not enough space in buffer.", 1);
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
    connection->error              = 1;
}

int
connection_send_network_data(connection* connection)
{
    int sent_bytes = 0;
    while(sent_bytes < connection->send_data_count)
    {
	    int send_size = (connection->send_data_count - sent_bytes);
        if(send_size > MAX_PACKET_SIZE)
        {
	        send_size = MAX_PACKET_SIZE;
        }
        
        int send_result =
            socket_send(connection->socket, connection->send_data + sent_bytes, send_size);
        // TODO: Handle send errors better. Only disconnect if actual error occurred, or
        // if the remote was disconnected.

        if(send_result > 0)
        {
            sent_bytes += send_result;
        }
        else
        {
            connection->send_data_count = 0;
            connection->pending_disconnect = 1;
            break;
        }
    }

    if(sent_bytes < connection->send_data_count)
    {
        char* offset = (connection->send_data + sent_bytes);
        int   length = (connection->send_data_count - sent_bytes);

        memmove(connection->send_data, offset, length);

        connection->send_data_count = length;
    }
    else
    {
        connection->send_data_count = 0;
    }
    
    return sent_bytes;
}

int
connection_recv_network_data(connection* connection, char* io_buffer, int io_buffer_size)
{
    int remaining_size = io_buffer_size;
    int received_bytes = 0;
    while(remaining_size > 0)
    {
        int recv_result = socket_recv(connection->socket, io_buffer + received_bytes, remaining_size);
        if(recv_result == -1) 
        {
            if(SOCKET_CHECK_ERROR_NO_PANIC() != 0)
            {
                connection->pending_disconnect = 1;
            }

            break;
        }
        else if(recv_result == 0)
        {
            connection->send_data_count = 0;
            connection->pending_disconnect = 1;
            break;
        }

        received_bytes += recv_result;
        remaining_size -= recv_result;
    }

    return received_bytes;
}

void
connection_transfer_prepare(connection_file_transfer* transfer, int64_t size)
{
	transfer->file_size       = size;
    transfer->byte_count_recv = 0;
    transfer->byte_count_disk = 0;

    transfer->io_buffer_size     = 0;
    transfer->io_buffer_capacity = MAX_PACKET_SIZE * 16;
    transfer->io_buffer          = malloc(MAX_PACKET_SIZE * 16);

    memset(&transfer->download_file, 0, sizeof(mapped_file));
}

void
connection_transfer_cancel(connection* connection)
{
    if(connection->transfer_in_progress || connection->transfer_completed)
    {
        connection_transfer_free(&connection->transfer);
        connection->transfer_in_progress = 0;
        connection->transfer_completed = 0;
    }
}


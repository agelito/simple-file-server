#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#define DEFAULT_SENDING_PORT 2771
#define DEFAULT_RECEIVE_PORT 2772
#define DEFAULT_LISTEN_PORT  2773

#define INET_STRADDR_LENGTH 128

#define MAX_PACKET_SIZE 4096

#define MAX_FILE_NAME		1024
#define MAX_FILE_CHUNK_SIZE 4096

#define PACKET_INVALID_PROTOCOL  0x1
#define PACKET_DISCONNECT        0x2
#define PACKET_FILE_UPLOAD_BEGIN 0x20
#define PACKET_FILE_UPLOAD_CHUNK 0x21
#define PACKET_FILE_UPLOAD_FINAL 0x22

#pragma pack(push, 1)
typedef struct packet_header
{
    char  packet_type;
    short packet_size;
} packet_header;

typedef struct packet_file_upload_begin
{
	int file_size;
	int file_name_length;
	int chunk_count;
	
} packet_file_upload_begin;

typedef struct packet_file_upload_chunk
{
	int chunk_size;
} packet_file_upload_chunk;
#pragma pack(pop)

#endif // PROTOCOL_H_INCLUDED

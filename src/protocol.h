#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#define DEFAULT_SENDING_PORT 2771
#define DEFAULT_RECEIVE_PORT 2772
#define DEFAULT_LISTEN_PORT  2773

#define INET_STRADDR_LENGTH 128

#define MAX_PACKET_SIZE 4096

#define PACKET_INVALID_PROTOCOL  0x1
#define PACKET_DISCONNECT        0x2
#define PACKET_FILE_UPLOAD_BEGIN 0x20
#define PACKET_FILE_UPLOAD_CHUNK 0x21
#define PACKET_FILE_UPLOAD_FINAL 0x22

typedef struct packet_header
{
    int packet_type;
    int packet_size;
} packet_header;

#endif // PROTOCOL_H_INCLUDED

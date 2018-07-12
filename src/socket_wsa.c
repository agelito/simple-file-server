// socket_wsa.c

#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define CHECK_WSA_ERROR() if(check_wsa_error() != 0) panic(1)

void socket_set_nonblocking(SOCKET socket);

int 
check_wsa_error() 
{
    int error = WSAGetLastError();
    if(error == WSAEWOULDBLOCK)
    {
        return 0;
    }
    else if(error != 0) 
    {
        wchar_t message[1024];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      message, sizeof(message), NULL);

        printf("WSA Error:\n%S\n", message);
    }

    return error;
}

SOCKET
socket_create_udp()
{
    SOCKET created_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    CHECK_WSA_ERROR();

    socket_set_nonblocking(created_socket);

    return created_socket;
}

SOCKET
socket_create_tcp()
{
    SOCKET created_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_WSA_ERROR();

    socket_set_nonblocking(created_socket);

    return created_socket;
}

void
socket_enable_broadcast(SOCKET socket)
{
    int allow_broadcast = 1;
    setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char*)&allow_broadcast, sizeof(allow_broadcast));
    CHECK_WSA_ERROR();
}

void socket_set_nonblocking(SOCKET socket)
{
    u_long blocking = 1;
    ioctlsocket(socket, FIONBIO, &blocking);
    CHECK_WSA_ERROR();
}

void
socket_address_to_string(struct sockaddr_in* address, wchar_t* string_buffer, int string_buffer_length)
{
    WSAAddressToString((LPSOCKADDR)address, sizeof(struct sockaddr_in), 0, string_buffer, (LPDWORD)&string_buffer_length);
}

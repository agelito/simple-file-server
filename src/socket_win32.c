// socket_win32.c

#include "platform.h"

#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

int 
socket_check_error() 
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

void
socket_initialize()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	SOCKET_CHECK_ERROR();
}

void
socket_shutdown()
{
	WSACleanup();
}

socket_handle
socket_create_udp()
{
	SOCKET created_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	SOCKET_CHECK_ERROR();

    socket_set_nonblocking(created_socket);

    return (socket_handle)created_socket;
}

socket_handle
socket_create_tcp()
{
	SOCKET created_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKET_CHECK_ERROR();

	socket_set_nonblocking(created_socket);

	return (socket_handle)created_socket;
}

void
socket_close(socket_handle socket)
{
	closesocket((SOCKET)socket);
}

void
socket_enable_broadcast(socket_handle socket)
{
	int allow_broadcast = 1;
	setsockopt((SOCKET)socket, SOL_SOCKET, SO_BROADCAST,
	           (char*)&allow_broadcast, sizeof(allow_broadcast));
	SOCKET_CHECK_ERROR();
}

void socket_set_nonblocking(socket_handle socket)
{
	u_long blocking = 1;
	ioctlsocket((SOCKET)socket, FIONBIO, &blocking);
	SOCKET_CHECK_ERROR();
}

void
socket_address_to_string(struct sockaddr_in* address, wchar_t* string_buffer, int string_buffer_length)
{
    WSAAddressToString((LPSOCKADDR)address, sizeof(struct sockaddr_in), 0,
                       string_buffer, (LPDWORD)&string_buffer_length);
}

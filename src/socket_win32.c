// socket_win32.c

#include "platform.h"

#define FD_SETSIZE MAX_CONNECTION_COUNT+1
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

int 
socket_check_error(char* function) 
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

        printf("WSA Error (%s):\n%S\n", function, message);
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
socket_cleanup()
{
	WSACleanup();
}

socket_handle
socket_create_udp()
{
	SOCKET created_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	SOCKET_CHECK_ERROR();

    socket_set_nonblocking((socket_handle)created_socket);

    return (socket_handle)created_socket;
}

socket_handle
socket_create_tcp()
{
	SOCKET created_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKET_CHECK_ERROR();

	socket_set_nonblocking((socket_handle)created_socket);

	return (socket_handle)created_socket;
}

void
socket_close(socket_handle socket)
{
	int close_result = closesocket((SOCKET)socket);
    if(close_result != 0) SOCKET_CHECK_ERROR_NO_PANIC();
}

void
socket_enable_broadcast(socket_handle socket)
{
	int allow_broadcast = 1;
	setsockopt((SOCKET)socket, SOL_SOCKET, SO_BROADCAST,
	           (char*)&allow_broadcast, sizeof(allow_broadcast));
	SOCKET_CHECK_ERROR_NO_PANIC();
}

void 
socket_set_nonblocking(socket_handle socket)
{
	u_long blocking = 1;
	ioctlsocket((SOCKET)socket, FIONBIO, &blocking);
	SOCKET_CHECK_ERROR_NO_PANIC();
}

void
socket_set_blocking(socket_handle socket)
{
    u_long blocking = 0;
	ioctlsocket((SOCKET)socket, FIONBIO, &blocking);
	SOCKET_CHECK_ERROR_NO_PANIC();
}

void
socket_set_nolinger(socket_handle socket)
{
    int optval = 0;
    int opt_result = setsockopt((SOCKET)socket, SOL_SOCKET, SO_DONTLINGER, (const char*)&optval, sizeof(int));
    if(opt_result == SOCKET_ERROR) SOCKET_CHECK_ERROR_NO_PANIC();
}

void
socket_address_to_string(socket_address* address, char* string_buffer, int string_buffer_length)
{
    WSAAddressToStringA((LPSOCKADDR)address, sizeof(struct sockaddr_in), 0,
                       string_buffer, (LPDWORD)&string_buffer_length);
}

socket_address
socket_create_inet_address(char* ip, int port)
{
	socket_address result;
	memset(&result, 0, sizeof(socket_address));

	struct sockaddr_in* addr_in = (struct sockaddr_in*)&result;
	addr_in->sin_family         = AF_INET;
	addr_in->sin_port           = htons((u_short)port);
    addr_in->sin_addr.s_addr    = inet_addr(ip);
	
	return result;
}

void
socket_inet_address_set_addr(socket_address* destination, int addr)
{
    struct sockaddr_in* addr_in = (struct sockaddr_in*)destination;
    addr_in->sin_addr.s_addr = addr;
}

int
socket_inet_address_get_addr(socket_address* source)
{
    struct sockaddr_in* addr_in = (struct sockaddr_in*)source;
    return addr_in->sin_addr.s_addr;
}

void
socket_bind(socket_handle socket, socket_address address)
{
    bind(socket, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
    SOCKET_CHECK_ERROR();
}

void
socket_listen(socket_handle socket)
{
    listen(socket, SOMAXCONN);
    SOCKET_CHECK_ERROR();
}

int
socket_recvfrom(socket_handle socket, char* recv_buffer, int recv_buffer_size,
                                socket_address* address)
{
    int address_length = sizeof(struct sockaddr_in);
	int receive_result = recvfrom(socket, recv_buffer, recv_buffer_size, 0,
	                              (struct sockaddr*)address, &address_length);
	if(receive_result == -1) SOCKET_CHECK_ERROR();
	return receive_result;
}

int
socket_send(socket_handle socket, char* send_buffer, int send_length)
{
	int send_result = send(socket, send_buffer, send_length, 0);
	if(send_result == -1) SOCKET_CHECK_ERROR();
	return send_result;
}

int
socket_recv(socket_handle socket, char* recv_buffer, int recv_length)
{
	int recv_result = recv(socket, recv_buffer, recv_length, 0);
	if(recv_result == -1) SOCKET_CHECK_ERROR_NO_PANIC();
	return recv_result;
}

int
socket_sendto(socket_handle socket, char* send_buffer, int send_buffer_size, socket_address* address)
{
	int send_result = sendto(socket, send_buffer, send_buffer_size, 0,
	                         (struct sockaddr*)address, sizeof(struct sockaddr_in));
	if(send_result == -1) SOCKET_CHECK_ERROR();
	return send_result;
}

socket_handle
socket_accept(socket_handle socket, socket_address* address)
{
	int address_length = sizeof(struct sockaddr_in);
	socket_handle accepted = (socket_handle)accept((SOCKET)socket, (struct sockaddr*)address, &address_length);
	if(accepted == INVALID_SOCKET) 
    {
        SOCKET_CHECK_ERROR();
        accepted = 0;
    }
	return accepted;
}

void
socket_connect(socket_handle socket, socket_address* address)
{
    int address_length = sizeof(struct sockaddr_in);
    int connect_result = connect((SOCKET)socket, (struct sockaddr*)address, address_length);
    if(connect_result == SOCKET_ERROR) SOCKET_CHECK_ERROR();
}

void
socket_shutdown(socket_handle socket)
{
    shutdown(socket, SD_BOTH);
}

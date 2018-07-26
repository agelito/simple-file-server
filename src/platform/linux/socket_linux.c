// socket_linux.c

#include "../platform.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/udp.h>

int
socket_check_error(char* function)
{
	int error = errno;
	if(error == EWOULDBLOCK || error == EAGAIN)
		return 0;
 	else if(error == EINPROGRESS)
		return 0;
	else if(error != 0)
	{
		char* error_string = strerror(error);
		printf("Error: (%s)(%d) %s\n", function, error, error_string);
	}

	return error;
}

void
socket_initialize()
{

}

void
socket_cleanup()
{

}

socket_handle
socket_create_udp()
{
	int created_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(created_socket == -1) SOCKET_CHECK_ERROR();

	return (socket_handle)created_socket;
}

socket_handle
socket_create_tcp()
{
	int created_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(created_socket == -1) SOCKET_CHECK_ERROR();

	return (socket_handle)created_socket;
}

void
socket_close(socket_handle socket)
{
	close(socket);
}

void
socket_set_nonblocking(socket_handle socket)
{
	int nonblocking_on = 1;
	ioctl(socket, FIONBIO, (char *)&nonblocking_on);
}

void
socket_set_blocking(socket_handle socket)
{
	int nonblocking_on = 0;
	ioctl(socket, FIONBIO, (char *)&nonblocking_on);
}

void
socket_enable_broadcast(socket_handle socket)
{
	int allow_broadcast = 1;
	setsockopt(socket, SOL_SOCKET, SO_BROADCAST,
	           &allow_broadcast, sizeof(allow_broadcast));
	SOCKET_CHECK_ERROR();
}

void
socket_address_to_string(socket_address* address, char* string_buffer,
                         int string_buffer_length)
{
	struct sockaddr_in* addr_in = (struct sockaddr_in*)address;
	char* address_string = inet_ntoa(addr_in->sin_addr);

	int port = ntohs(addr_in->sin_port);
	snprintf(string_buffer, string_buffer_length, "%s:%d", address_string, port);
}

socket_address
socket_create_inet_address(char* ip, int port)
{
	socket_address result;
	memset(&result, 0, sizeof(socket_address));

	struct sockaddr_in* addr_in = (struct sockaddr_in*)&result;
	addr_in->sin_family = AF_INET;
	addr_in->sin_port	= htons(port);
	inet_aton(ip, &addr_in->sin_addr);
	
	return result;
}

void
socket_inet_address_set_addr(socket_address* destination, int addr)
{
	struct sockaddr_in* addr_dst = (struct sockaddr_in*)destination;
	addr_dst->sin_addr.s_addr = addr;
}

int
socket_inet_address_get_addr(socket_address* source)
{
	struct sockaddr_in* addr_src = (struct sockaddr_in*)source;
	return addr_src->sin_addr.s_addr;
}

void
socket_bind(socket_handle socket, socket_address address)
{
	int bind_result = bind(socket, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
	if(bind_result == -1) SOCKET_CHECK_ERROR();
}

void
socket_listen(socket_handle socket)
{
	int listen_result = listen(socket, SOMAXCONN);
	if(listen_result == -1) SOCKET_CHECK_ERROR();
}

int
socket_recvfrom(socket_handle socket, char* recv_buffer, int recv_buffer_size,
                socket_address* address)
{
	unsigned int address_length = sizeof(struct sockaddr_in);
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
	if(recv_result == -1)
	{
		SOCKET_CHECK_ERROR();
	}
	else if(recv_result == 0)
	{
		// NOTE: Linux sockets return 0 if the socket was orderly shutdown. Non-blocking sockets
		// would return -1 and error would indicate that socket is waiting for data.
		recv_result = -1;
	}
	
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
	unsigned int address_length = sizeof(struct sockaddr_in);
	socket_handle accepted = accept(socket, (struct sockaddr*)address, &address_length);
	if(accepted == -1) SOCKET_CHECK_ERROR();
	return accepted;
}

void
socket_connect(socket_handle socket, socket_address* address)
{
	int address_length = sizeof(struct sockaddr_in);
	int connect_result = connect(socket, (struct sockaddr*)address, address_length);
	if(connect_result == -1) SOCKET_CHECK_ERROR();
}

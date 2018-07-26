#ifndef PLATFORM_H_INCLUDED
#define PLATFORM_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define UNUSED(parameter) (void)(parameter)

#ifdef _WIN32
#define CURRENT_FUNCTION __FUNCTION__
#else
#define CURRENT_FUNCTION __func__
#endif

#define MAX_CONNECTION_COUNT 128

#define SOCKET_CHECK_ERROR() if(socket_check_error((char*)CURRENT_FUNCTION) != 0) panic(1)
#define SOCKET_CHECK_ERROR_NO_PANIC() socket_check_error((char*)CURRENT_FUNCTION)

extern char platform_path_delimiter;
extern int platform_quit;

typedef uint32_t socket_handle;
typedef uint32_t file_handle;

typedef struct timer {
    int64_t       performance_frequency;
    int64_t       performance_counter;
    uint64_t      rdtsc_cycles;
    uint64_t      frame_cycles;
    int64_t       elapsed_counter;
    double        delta_milliseconds;
	double        elapsed_seconds;
    double        frames_per_second;
    double        megacycles_per_frame;
    uint64_t      frame_counter;
} timer;

typedef struct measure_time {
    int64_t performance_frequency;
    int64_t performance_counter;
    double  delta_time;
} measure_time;

typedef struct socket_address {
	uint8_t opaque_data[24];
} socket_address;

typedef struct selectable_set {
	void* read;
	void* write;
} selectable_set;

typedef struct mapped_file {
    char*       file_path;
    file_handle file;
    file_handle mapping;
    int         read_only;
    int64_t     file_size;
    int64_t     offset;
} mapped_file;

typedef struct mapped_file_view {
    void*   base;
    void*   mapped;
    int64_t size_aligned;
    int64_t size;
} mapped_file_view;

void panic(int exit_code);

void thread_sleep();

void timer_initialize(timer* timer);
void timer_reset_accumulators(timer* timer);
void timer_end_frame(timer* timer);

void measure_initialize(measure_time* measure);
void measure_tick(measure_time* measure);

double   time_get_seconds();
uint64_t time_get_nanoseconds();
uint64_t time_get_cycles();

void console_init();
void console_clear();

int				socket_check_error(char* function);
void			socket_initialize();
void			socket_cleanup();
socket_handle	socket_create_udp();
socket_handle	socket_create_tcp();
void			socket_close(socket_handle socket);
void			socket_set_nonblocking(socket_handle socket);
void            socket_set_blocking(socket_handle socket);
void            socket_set_nolinger(socket_handle socket);
void			socket_enable_broadcast(socket_handle socket);
void			socket_address_to_string(socket_address* address, char* string_buffer,
			                             int string_buffer_length);
socket_address  socket_create_inet_address(char* ip, int port);
void            socket_inet_address_set_addr(socket_address* destination, int addr);
int             socket_inet_address_get_addr(socket_address* source);
void            socket_bind(socket_handle socket, socket_address address);
void            socket_listen(socket_handle socket);
int             socket_recvfrom(socket_handle socket, char* recv_buffer, int recv_buffer_size,
                                socket_address* address);
int             socket_send(socket_handle socket, char* send_buffer, int send_length);
int             socket_recv(socket_handle socket, char* recv_buffer, int recv_length);
int             socket_sendto(socket_handle socket, char* send_buffer, int send_buffer_size,
                              socket_address* address);
socket_handle   socket_accept(socket_handle socket, socket_address* address);
void            socket_connect(socket_handle socket, socket_address* address);
void            socket_shutdown(socket_handle socket);

selectable_set	selectable_set_create();
int  			selectable_set_can_read(selectable_set* set, socket_handle socket);
int	     		selectable_set_can_write(selectable_set* set, socket_handle socket);
void			selectable_set_set_read(selectable_set* set, socket_handle socket);
void			selectable_set_set_write(selectable_set* set, socket_handle socket);
void			selectable_set_clear(selectable_set* set);
void			selectable_set_destroy(selectable_set* set);
int             selectable_set_select(selectable_set* set, int highest_handle, int timeout_milliseconds);
int             selectable_set_select_noblock(selectable_set* set, int highest_handle);

void exe_set_working_directory(char* directory_path);
void exe_get_directory(char* output, int output_length);

long platform_format(char* destination, long size, char* format, ...);

void        filesystem_create_directory(char* directory_path);
void        filesystem_delete_directory(char* directory_path);
int         filesystem_directory_exists(char* directory_path);
int         filesystem_open_create_file(char* file_path);
int         filesystem_open_read_file(char* file_path);
void        filesystem_close_file(int file_handle);
void        filesystem_delete_file(char* file_path);
int         filesystem_file_exists(char* file_path);
mapped_file filesystem_create_mapped_file(char* file_path, int read_only, int64_t size);
void        filesystem_destroy_mapped_file(mapped_file* mapped_file);
mapped_file_view filesystem_file_view_map(mapped_file* mapped_file, int64_t size);
void             filesystem_file_view_unmap(mapped_file_view* view);


#endif // PLATFORM_H_INCLUDED

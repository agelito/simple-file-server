// selectable_linux.c

selectable_set
selectable_set_create()
{
	selectable_set set;
	set.read = malloc(sizeof(fd_set));
	memset(set.read, 0, sizeof(fd_set));
	set.write = malloc(sizeof(fd_set));
	memset(set.write, 0, sizeof(fd_set));
	return set;
}

int
selectable_set_can_read(selectable_set* set, socket_handle socket)
{
	return FD_ISSET(socket, (fd_set*)set->read);
}

int
selectable_set_can_write(selectable_set* set, socket_handle socket)
{
	return FD_ISSET(socket, (fd_set*)set->write);
}

void
selectable_set_set_read(selectable_set* set, socket_handle socket)
{
	FD_SET(socket, (fd_set*)set->read);
}

void
selectable_set_set_write(selectable_set* set, socket_handle socket)
{
	FD_SET(socket, (fd_set*)set->write);
}

void
selectable_set_clear(selectable_set* set)
{
	FD_ZERO((fd_set*)set->read);
	FD_ZERO((fd_set*)set->write);
}

void
selectable_set_destroy(selectable_set* set)
{
	selectable_set_clear(set);

	free(set->read);
	set->read = 0;

	free(set->write);
	set->write = 0;
}

#define MILLISECONDS_TO_MICROSECONDS 1000

int selectable_set_select(selectable_set* set, int highest_handle, int timeout_milliseconds)
{
	struct timeval select_timeout;
	select_timeout.tv_sec = 0;
	select_timeout.tv_usec = timeout_milliseconds * MILLISECONDS_TO_MICROSECONDS;
	
	return select(highest_handle + 1, (fd_set*)set->read, (fd_set*)set->write, 0, &select_timeout);
}

int selectable_set_select_noblock(selectable_set* set, int highest_handle)
{
	struct timeval select_timeout;
	select_timeout.tv_sec = 0;
	select_timeout.tv_usec = 1;
	
	return select(highest_handle + 1, (fd_set*)set->read, (fd_set*)set->write, 0, &select_timeout);
}

// selectable_win32.c

selectable_set
selectable_set_create()
{
	selectable_set set;
	set.read = malloc(sizeof(FD_SET));
	memset(set.read, 0, sizeof(FD_SET));
	set.write = malloc(sizeof(FD_SET));
	memset(set.write, 0, sizeof(FD_SET));
	return set;
}

int
selectable_set_can_read(selectable_set* set, socket_handle socket)
{
	return FD_ISSET(socket, (FD_SET*)set->read);
}

int
selectable_set_can_write(selectable_set* set, socket_handle socket)
{
	return FD_ISSET(socket, (FD_SET*)set->write);
}

void
selectable_set_set_read(selectable_set* set, socket_handle socket)
{
	FD_SET(socket, (FD_SET*)set->read);
}

void
selectable_set_set_write(selectable_set* set, socket_handle socket)
{
	FD_SET(socket, (FD_SET*)set->write);
}

void
selectable_set_clear(selectable_set* set)
{
	FD_ZERO((FD_SET*)set->read);
	FD_ZERO((FD_SET*)set->write);
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
    UNUSED(highest_handle);

	struct timeval select_timeout;
	select_timeout.tv_sec = 0;
	select_timeout.tv_usec = timeout_milliseconds * MILLISECONDS_TO_MICROSECONDS;
	
	return select(0, (FD_SET*)set->read, (FD_SET*)set->write, 0, &select_timeout);
}

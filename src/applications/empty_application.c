// empty_application.c

#include "platform/platform.h"
#include "application.h"

void*
empty_application_create(char* command_line[], int command_line_count)
{
    UNUSED(command_line);
    UNUSED(command_line_count);

    return 0;
}

void
empty_application_destroy(void* state)
{
    UNUSED(state);
}

int
empty_application_tick(void* state, timer* timer)
{
    UNUSED(state);
    UNUSED(timer);

    return 1;
}

int 
main(int argc, char* argv[])
{
    application application;
    application.target_ups = 60;
    application.create = empty_application_create;
    application.destroy = empty_application_destroy;
    application.tick = empty_application_tick;
    application.print = 0;

    application_run(&application, argc, argv);

    return 0;
}

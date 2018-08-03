#ifndef APPLICATION_H_INCLUDED
#define APPLICATION_H_INCLUDED

typedef void* (*application_create)(char* command_line[], int command_line_count);
typedef void  (*application_destroy)(void* state);
typedef int   (*application_tick)(void* state, timer* timer);
typedef void  (*application_print)(void* state, timer* timer);

typedef struct application
{
    int                 target_ups;
    application_create  create;
    application_destroy destroy;
    application_tick    tick;
    application_print   print;
} application;

void 
application_run(application* application, int argc, char* argv[]);

#endif // APPLICATION_H_INCLUDED

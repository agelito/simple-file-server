// application.c

#include "platform/platform.h"

#include "application.h"

void
wait_for_target_ups(measure_time* measure, double target_delta)
{
    measure_tick(measure);

    int sleep_epsilon = 2;

    double accumulator = 0.0;
    while(accumulator < target_delta)
    {
        accumulator += measure->delta_time;

        double remaining = (target_delta - accumulator);
        
        int remaining_milliseconds = (int)(remaining * 1000);
        if(remaining_milliseconds > sleep_epsilon)
        {
            thread_sleep(remaining_milliseconds);
        }

        measure_tick(measure);
    }
}

void 
application_run(application* application, int argc, char* argv[])
{
    console_init();
    socket_initialize();

    timer timer;
    timer_initialize(&timer);

    measure_time measure_target_ups;
    measure_initialize(&measure_target_ups);

    if(!application->target_ups)
    {
        application->target_ups = 10;
    }

    double time_per_tick = 1.0 / application->target_ups;

    void* state = application->create(argv, argc);

    double last_print_time = timer.elapsed_seconds;

    console_clear();
    if(application->print)
        application->print(state, &timer);
    
    int running = 1;
    while(running && !platform_quit)
    {
        double time_since_print = (timer.elapsed_seconds - last_print_time);
        if(time_since_print >= 1.0f)
        {
            console_clear();

            if(application->print)
                application->print(state, &timer);

            last_print_time = timer.elapsed_seconds;

            timer_reset_accumulators(&timer);
        }

	    running = application->tick(state, &timer);

        wait_for_target_ups(&measure_target_ups, time_per_tick);

	    timer_end_frame(&timer);
    }

    application->destroy(state);

    socket_cleanup();
}

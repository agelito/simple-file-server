// timer_linux.c

#include <x86intrin.h> 

double
time_get_seconds()
{
	uint64_t nanoseconds = time_get_nanoseconds();
	double seconds = (double)nanoseconds / NANOSECOND;
	
	return seconds;
}

uint64_t
time_get_nanoseconds()
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC, &timespec);

    uint64_t nanoseconds = timespec.tv_sec * NANOSECOND;
    nanoseconds += timespec.tv_nsec;
    return nanoseconds;
}

void
timer_initialize(timer* timer)
{
	timer->performance_counter = time_get_nanoseconds();

    timer->rdtsc_cycles = __rdtsc();

    timer->frame_cycles = 0;
    timer->elapsed_counter = 0;
    timer->delta_milliseconds = 0.0;
    timer->elapsed_seconds = 0.0;
    timer->frames_per_second = 0.0;
    timer->megacycles_per_frame = 0.0;
    timer->frame_counter = 0;
}

void
timer_reset_accumulators(timer* timer)
{
    timer->delta_milliseconds = 0.0;
    timer->frames_per_second = 0.0;
    timer->megacycles_per_frame = 0.0;
    timer->frame_counter = 0;
}

void
timer_end_frame(timer* timer)
{
    uint64_t rdtsc_cycles = __rdtsc();

    uint64_t performance_counter = time_get_nanoseconds();
    
    timer->frame_cycles = rdtsc_cycles - timer->rdtsc_cycles;
    timer->rdtsc_cycles = rdtsc_cycles;

    timer->elapsed_counter = performance_counter - timer->performance_counter;
    timer->performance_counter = performance_counter;

    timer->delta_milliseconds += (1000.0 * ((double)timer->elapsed_counter / (double)NANOSECOND));
    timer->frames_per_second += (double)NANOSECOND / (double)timer->elapsed_counter;
    timer->megacycles_per_frame += ((double)timer->frame_cycles / (1000.0 * 1000.0));

    timer->elapsed_seconds += ((double)timer->elapsed_counter / (double)NANOSECOND);

    timer->frame_counter++;
}

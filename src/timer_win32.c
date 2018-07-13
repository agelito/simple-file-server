// timer.c

#include <intrin.h> 
#include <stdint.h>

double
time_get_seconds()
{
	FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);

    LONGLONG time_nano100 = (LONGLONG)file_time.dwLowDateTime + 
        ((LONGLONG)(file_time.dwHighDateTime) << 32LL);
    double seconds = (double)time_nano100 / 10000000.0;
    
    return seconds;
}

void
timer_initialize(timer* timer)
{
    LARGE_INTEGER performance_counter_frequency;
    QueryPerformanceFrequency(&performance_counter_frequency);
    timer->performance_frequency = performance_counter_frequency.QuadPart;

    LARGE_INTEGER performance_counter;
    QueryPerformanceCounter(&performance_counter);
    timer->performance_counter = performance_counter.QuadPart;

    timer->rdtsc_cycles = __rdtsc();

    timer->frame_cycles = 0;
    timer->elapsed_counter = 0;
    timer->delta_milliseconds = 0.0;
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
}

void
timer_end_frame(timer* timer)
{
    uint64_t rdtsc_cycles = __rdtsc();

    LARGE_INTEGER performance_counter;
    QueryPerformanceCounter(&performance_counter);

    timer->frame_cycles = rdtsc_cycles - timer->rdtsc_cycles;
    timer->rdtsc_cycles = rdtsc_cycles;

    timer->elapsed_counter = performance_counter.QuadPart - timer->performance_counter;
    timer->performance_counter = performance_counter.QuadPart;

    timer->delta_milliseconds += (((1000.0 * (double)timer->elapsed_counter) / (double)timer->performance_frequency));
    timer->frames_per_second += (double)timer->performance_frequency / (double)timer->elapsed_counter;
    timer->megacycles_per_frame += ((double)timer->frame_cycles / (1000.0 * 1000.0));

    timer->frame_counter++;
}

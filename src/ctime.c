// ctime.c

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define UNUSED(parameter) (void)(parameter)

int main(int argc, char* argv[]) 
{
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);

    LONGLONG time_nano100 = (LONGLONG)file_time.dwLowDateTime + 
        ((LONGLONG)(file_time.dwHighDateTime) << 32LL);
    double seconds = (double)time_nano100 / 10000000.0;

    if(argc > 1)
    {
        double seconds_from;
        sscanf(argv[1], "%lf", &seconds_from);
        seconds = (seconds - seconds_from);
    }

    printf("%lf", seconds);

    return 0;
}

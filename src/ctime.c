// ctime.c

#include <stdio.h>

#include "platform.h"

#define UNUSED(parameter) (void)(parameter)

int main(int argc, char* argv[]) 
{
	double seconds = time_get_seconds();

    if(argc > 1)
    {
        double seconds_from;
        sscanf(argv[1], "%lf", &seconds_from);
        seconds = (seconds - seconds_from);
    }

    printf("%lf", seconds);

    return 0;
}

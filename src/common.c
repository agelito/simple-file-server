// common.c

#define UNUSED(parameter) (void)(parameter)

void 
panic(int exit_code)
{
    printf("Panic! Exiting with code: %d.\n", exit_code);
    exit(exit_code);
}

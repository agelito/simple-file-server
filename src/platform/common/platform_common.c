// platform_common.c

void 
panic(int exit_code)
{
    printf("Panic! Exiting with code: %d.\n", exit_code);
    exit(exit_code);
}

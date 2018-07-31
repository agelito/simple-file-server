// platform_common.c

void 
panic(char* message, int exit_code)
{
    printf("Panic!\nMessage: %s\nExiting with code: %d.\n", message, exit_code);
    exit(exit_code);
}

// console_utils.c

static int console_indicate_quit = 0;

BOOL WINAPI 
console_handler(DWORD signal) 
{
    switch(signal)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        console_indicate_quit = 1;
        break;
    }

    return TRUE;
}

void
console_utils_init()
{
    SetConsoleCtrlHandler(console_handler, TRUE);
}

void
clear_console(HANDLE console)
{
    COORD cursor_position = { 0, 0 };
    DWORD characters_written = 0;
    CONSOLE_SCREEN_BUFFER_INFO console_info; 
    DWORD console_size = 0;

    if(!GetConsoleScreenBufferInfo(console, &console_info))
        return;

    console_size = console_info.dwSize.X * console_info.dwSize.Y;

    if(!FillConsoleOutputCharacter(console, (TCHAR)' ', console_size, cursor_position, &characters_written))
        return;

    if(!GetConsoleScreenBufferInfo(console, &console_info))
        return;

    if(!FillConsoleOutputAttribute(console, console_info.wAttributes, console_size, cursor_position, &characters_written))
        return;

    SetConsoleCursorPosition(console, cursor_position);
}



@echo off

tools\ctime.exe > cs.time

cl /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\server_locator.exe src\server_locator.c src\platform_win32.c
cl /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\fileserver.exe src\fileserver.c src\platform_win32.c

set /p COMPILE_START=<cs.time
tools\ctime.exe %COMPILE_START% > ce.time
set /p COMPILE_TIME=<ce.time

echo Compilation took %COMPILE_TIME% seconds.

del cs.time
del ce.time


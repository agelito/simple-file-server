@echo off

tools\ctime.exe > cs.time

cl /Isrc /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\server_locator.exe src\applications\server_locator.c src\platform\platform_win32.c
cl /Isrc /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\fileserver.exe src\applications\fileserver.c src\application.c src\platform\platform_win32.c
cl /Isrc /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\stress_test.exe src\applications\test\stress_test.c src\platform\platform_win32.c
cl /Isrc /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\map_file_test.exe src\applications\test\map_file_test.c src\platform\platform_win32.c
cl /Isrc /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\upload_file.exe src\applications\upload_file.c src\platform\platform_win32.c
cl /Isrc /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmbin\ /Febin\empty_application.exe src\applications\empty_application.c src\application.c src\platform\platform_win32.c


set /p COMPILE_START=<cs.time
tools\ctime.exe %COMPILE_START% > ce.time
set /p COMPILE_TIME=<ce.time

echo Compilation took %COMPILE_TIME% seconds.

del cs.time
del ce.time


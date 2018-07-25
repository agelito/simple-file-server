@echo off

cl /Isrc /DUNICODE /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmtools\ /Fetools\ctime.exe src\tools\ctime.c src\platform\platform_win32.c
@echo off

cl /D_CRT_SECURE_NO_WARNINGS /EHsc- /fp:fast /GS- /MP8 /O2 /W4 /WX /MT /Foobj\ /Fmtools\ /Fetools\ctime.exe src\ctime.c
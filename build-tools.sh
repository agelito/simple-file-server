#!/bin/sh

gcc -std=c99 -o tools/ctime -Wall -O3 -pedantic -lrt -Isrc src/tools/ctime.c src/platform/platform_linux.c

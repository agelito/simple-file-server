#!/bin/sh

gcc -std=c99 -o tools/ctime -Wall -O3 -pedantic -lrt src/ctime.c src/platform_linux.c

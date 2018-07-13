#!/bin/sh

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "${BASE_DIR}"

COMPILE_START=$(./tools/ctime)

gcc -std=c99 -o bin/server_locator -Wall -O3 -pedantic -lrt src/server_locator.c src/platform_linux.c
gcc -std=c99 -o bin/fileserver -Wall -O3 -pedantic -lrt src/fileserver.c src/platform_linux.c

COMPILE_TIME=$(./tools/ctime ${COMPILE_START})
echo "Compilation took ${COMPILE_TIME} seconds."


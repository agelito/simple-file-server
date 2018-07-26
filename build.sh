#!/bin/sh

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "${BASE_DIR}"

COMPILE_START=$(./tools/ctime)

gcc -std=c99 -o bin/server_locator -Wall -O3 -pedantic -lrt -Isrc src/applications/server_locator.c src/platform/platform_linux.c
gcc -std=c99 -o bin/fileserver -Wall -O0 -g -pedantic -lrt -Isrc src/applications/fileserver.c src/platform/platform_linux.c
gcc -std=c99 -o bin/upload_file -Wall -O3 -pedantic -lrt -Isrc src/applications/upload_file.c src/platform/platform_linux.c
gcc -std=c99 -o bin/stress_test -Wall -O3 -pedantic -lrt -Isrc src/applications/test/stress_test.c src/platform/platform_linux.c
gcc -std=c99 -o bin/map_file_test -Wall -O3 -pedantic -lrt -Isrc src/applications/test/map_file_test.c src/platform/platform_linux.c


COMPILE_TIME=$(./tools/ctime ${COMPILE_START})
echo "Compilation took ${COMPILE_TIME} seconds."


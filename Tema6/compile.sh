#! /bin/bash

rm build/ -rf

mkdir build

gcc daytime-tcp-client-Rebe-Martin.c -o build/daytime-tcp-client-Rebe-Martin -Wall -Werror
gcc daytime-tcp-server-Rebe-Martin.c -o build/daytime-tcp-server-Rebe-Martin -Wall -Werror


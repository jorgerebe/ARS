#! /bin/bash

rm build/ -rf

mkdir build

gcc tftp-Rebe-Martin.c -o build/tftp-client-Rebe-Martin -Wall -Werror

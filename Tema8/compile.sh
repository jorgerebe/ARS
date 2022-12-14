#! /bin/bash

rm build/ -rf

mkdir build

gcc miping-Rebe-Martin.c -o build/miping-Rebe-Martin -Wall -Werror

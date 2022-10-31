#! /bin/bash

for x in {1..200}
do
	./build/daytime-tcp-client-Rebe-Martin 127.0.0.1
	echo $x
done

#! /bin/bash

for x in {1..20000}
do
	./cliente 127.0.0.1
	echo $x
done

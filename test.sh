#!/usr/bin/sh

rm result
for i in $(seq 1 1000)
do
	#timeout --signal='SIGHUP' 2s ./threadSwitch.o
	./threadSwitch.o
done
./visual.py

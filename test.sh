#!/usr/bin/sh

for i in $(seq 1 10000)
do
	#timeout --signal='SIGHUP' 2s ./threadSwitch.o
	./threadSwitch.o
done
./visual.py

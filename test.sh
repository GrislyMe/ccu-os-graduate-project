#!/usr/bin/sh

rm result
for i in $(seq 1 10000)
do
	#timeout --signal='SIGHUP' 2s ./threadSwitch.o
	nice -n-20 ./threadSwitch
done
./visual.py

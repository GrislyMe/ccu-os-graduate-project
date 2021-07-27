#!/bin/sh

rm result
nice -n-20 ./threadSwitch
./visual.py

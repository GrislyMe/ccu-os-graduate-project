#!/bin/sh

rm result
sudo nice -n-20 ./threadSwitch
./visual.py

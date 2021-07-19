#!/usr/bin/sh

timeout --signal='SIGHUP' 5s ./while_test.o > result
./visual.py
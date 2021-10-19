#!/bin/zsh

for i in {1..100};
do
	echo $i
	./MCS_lock_test_RS
done

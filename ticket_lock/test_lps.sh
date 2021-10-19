#!/bin/zsh

for i in {1..100};
do
	echo $i
	./ticket_lock_test_RS
done

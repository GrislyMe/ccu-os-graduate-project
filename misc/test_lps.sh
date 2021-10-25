#!/bin/zsh

rm result
for i in {1..100};
do
	echo $i
	./threadSwitch
done

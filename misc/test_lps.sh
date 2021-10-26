#!/bin/zsh

rm result
for i in {1..1000};
do
	echo $i
	./threadSwitch
done

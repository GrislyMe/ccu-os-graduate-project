#!/bin/zsh

rm ./ticket_lock/ticket_lps
rm ./plock/plock_lps
rm ./mcs/mcs_lps
rm ./SoA/SoA_lps

for i in {1..10};
do
	echo $i
	./ticket_lock/ticket_lock_test_RS
	./SoA/routing_spinlock
	./plock/plock_rs
	./mcs/MCS_lock_test_RS
done

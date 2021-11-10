
.PHONY: all
all:
	+$(MAKE) -C mcs
	+$(MAKE) -C plock
	+$(MAKE) -C ticket_lock
	+$(MAKE) -C SoA

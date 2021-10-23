SHELL = /bin/bash
CC = gcc
CFLAGS = -std=gnu11 -pthread -Wall -O3 -latomic
LIB = ../lib/soa_lock.c ../lib/util.c
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %, $(SRC))

.PHONY: all
all: $(EXE)

%: %.c
	$(CC) $(CFLAGS) $(LIB) $*.c -o $@

.PHONY: clean
clean: $(EXE)
	rm $(EXE)

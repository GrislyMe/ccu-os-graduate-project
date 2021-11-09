SHELL = /bin/bash
CC = gcc
CFLAGS = -std=gnu11 -pthread -Wall -O3 -latomic
LIB = ../lib/ticket_lock.c ../lib/util.c
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %, $(SRC))

.PHONY: all
all: $(EXE)

%: %.c $(LIB)
	$(CC) $(CFLAGS) $(LIB) $*.c -o $@

.PHONY: clean
clean: $(EXE)
	rm $(EXE)

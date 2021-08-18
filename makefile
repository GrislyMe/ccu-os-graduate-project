SHELL = /bin/bash
CC = gcc
CFLAGS = -std=gnu11 -pthread -Wall -O3 -latomic
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %, $(SRC))

all: ${EXE}

%: %.c
	${CC} ${CFLAGS} $*.c -o $@

clean: ${EXE}
	rm ${EXE}

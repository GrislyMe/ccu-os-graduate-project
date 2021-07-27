SHELL = /bin/bash
CC = gcc
CFLAGS = -std=c11 -pthread -Wall -O3
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %, $(SRC))

all: ${EXE}

%: %.c
	${CC} ${CFLAGS} $*.c -o $@

clean: ${EXE}
	rm ${EXE}
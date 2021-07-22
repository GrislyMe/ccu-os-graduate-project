SHELL = /bin/bash
CC = gcc
CFLAGS = -std=c11 -pthread -march=znver1 -Wall -O3
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %, $(SRC))

all: ${EXE}

%:	%.c
	${CC} ${CFLAGS} $@.c -o $@.o

test:
	./test.sh

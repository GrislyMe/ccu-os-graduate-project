SHELL = /bin/bash
CC = gcc
CFLAGS = -pthread -march=znver1 -O3
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %, $(SRC))

all: ${EXE}

%:	%.c
	${CC} ${CFLAGS} $@.c -o $@.o

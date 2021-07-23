SHELL = /bin/bash
CC = gcc
CFLAGS = -std=c11 -pthread -march=znver1 -Wall -O3
SRC = $(wildcard *.c)
EXE = $(patsubst %.c, %.o, $(SRC))

all: ${EXE}

%.o: %.c
	${CC} ${CFLAGS} $@.c -o $@.o
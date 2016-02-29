CFLAGS = -pg -Wall -O3
CC = c11

.PHONY: clean

all: main.c huffman.c
	gcc -o main main.c huffman.c prog_bar.c -pthread -I. $(CFLAGS) -std=c99 

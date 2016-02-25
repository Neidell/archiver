CFLAGS = -pg -Wall -O3
CC = c11

.PHONY: clean

all: main.c qtree.c
	gcc -o main main.c qtree.c prog_bar.c -pthread -I. $(CFLAGS) -std=c99 

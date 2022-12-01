CC = gcc
CFLAGS = -g -Wall -lm
 
all: build

build:
	$(CC) -o quadtree main.c $(CFLAGS) -std=c99 

.PHONY : clean
clean :
	rm -f quadtree


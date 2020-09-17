CC = gcc
CFLAGS = Wall

all: ask2-fork.o proc-common.o
	$(CC) -$(CFLAGS) ask2-fork.o proc-common.o -o fork-tree

proc-common.o: proc-common.c
	$(CC) -$(CFLAGS) -c proc-common.c

ask2-fork.o: ask2-fork.c
	$(CC) -$(CFLAGS) -c ask2-fork.c

clean:
	rm ask2-fork.o proc-common.o fork-tree

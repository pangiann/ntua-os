CC = gcc
CFLAGS = -g -Wall -O2

all: ask2-signals.o proc-common.o tree.o
	$(CC) $(CFLAGS) ask2-signals.o proc-common.o tree.o -o fork-signal

tree.o: tree.c
	$(CC) $(CFLAGS) -c tree.c

proc-common.o: proc-common.c
	$(CC) $(CFLAGS) -c proc-common.c

ask2-singals.o: ask2-signals.c
	$(CC) $(CFLAGS) -c ask2-signals.c

clean:
	rm ask2-signals.o proc-common.o tree.o fork-signal


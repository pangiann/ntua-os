CC = gcc
CFLAGS = -Wall

all: expr-tree.o proc-common.o tree.o
	$(CC) $(CFLAGS) expr-tree.o proc-common.o tree.o -o expr-tree

tree.o: tree.c
	$(CC) $(CFLAGS) -c tree.c

proc-common.o: proc-common.c
	$(CC) $(CFLAGS) -c proc-common.c

arithmetical-expression-tree.o: expr-tree.c
	$(CC) $(CFLAGS) -c expr-tree.c

clean:
	rm expr-tree.o proc-common.o tree.o expr-tree

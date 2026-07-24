CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Iinclude
LDFLAGS ?= -pthread

SRCS = src/ppool.c src/ppool_queue.c src/ppool_errno.c

.PHONY: all clean

all: libember_ppool.a ember_ppool_example

libember_ppool.a: $(SRCS)
	$(CC) $(CFLAGS) -c src/ppool.c -o ppool.o
	$(CC) $(CFLAGS) -c src/ppool_queue.c -o ppool_queue.o
	$(CC) $(CFLAGS) -c src/ppool_errno.c -o ppool_errno.o
	ar rcs $@ ppool.o ppool_queue.o ppool_errno.o

ember_ppool_example: example.c libember_ppool.a
	$(CC) $(CFLAGS) example.c -o $@ libember_ppool.a $(LDFLAGS)

clean:
	rm -f *.o libember_ppool.a ember_ppool_example

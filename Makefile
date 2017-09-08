
CC=gcc
CFLAGS=-Wall -O2 -DNDEBUG

BINS=libmyalloc.so


all: $(BINS)

libmyalloc.so:  allocator.c
	$(CC) $(CFLAGS) -fPIC -shared allocator.c -o libmyalloc.so -lm

clean:
	rm $(BINS)

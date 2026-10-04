CC=gcc
CFLAGS=-Wall -O2
LDFLAGS=

.PHONY: all clean

all: bwtsearch

bwtsearch: bwtsearch.c rle.c index.c search.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f bwtsearch

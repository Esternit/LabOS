CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L
LDFLAGS := -pthread -lrt

.PHONY: all clean

all: lab9_1 sender receiver

lab9_1: lab9_1.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sender: sender.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

receiver: receiver.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f lab9_1 sender receiver
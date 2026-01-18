CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L
LDFLAGS_PTHREAD := -pthread
LDFLAGS_SEM     := -pthread -lrt

.PHONY: all clean

all: lab9_1 sender receiver

lab9_1: lab9_1.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_PTHREAD)

sender: sender.c
	$(CC) $(CFLAGS) -o $@ $<

receiver: receiver.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f lab9_1 sender receiver ipc91.key ipc92.key
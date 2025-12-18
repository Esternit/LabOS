CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra
LDFLAGS := -pthread

.PHONY: all clean

all: lab9_1 sender receiver

lab9_1: lab9_1.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sender: sender.c
	$(CC) $(CFLAGS) -o $@ $<

receiver: receiver.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f lab9_1 sender receiver ipc91.key ipc92.key
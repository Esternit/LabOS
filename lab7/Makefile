CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lrt -lpthread

TARGETS = sender receiver

.PHONY: all clean

all: $(TARGETS)

sender: sender.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

receiver: receiver.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
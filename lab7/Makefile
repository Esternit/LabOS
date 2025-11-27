CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lrt

TARGETS = sender receiver

.PHONY: all clean

all: $(TARGETS)

sender: sender.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

receiver: receiver.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) /tmp/sender.lock /tmp/time_data.txt

veryclean: clean
	rm -f *.o
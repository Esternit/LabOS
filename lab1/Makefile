CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGETS = mycat mygrep

.PHONY: all clean

all: $(TARGETS)

mycat: mycat.c
	$(CC) $(CFLAGS) -o mycat mycat.c

mygrep: mygrep.c
	$(CC) $(CFLAGS) -o mygrep mygrep.c

clean:
	rm -f $(TARGETS)
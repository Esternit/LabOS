CC = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2

TARGETS = pipe_fork_example writer reader

SRCS = pipe_fork_example.c writer.c reader.c

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

clean-fifo:
	rm -f /tmp/example_fifo

distclean: clean clean-fifo

pipe_fork_example: pipe_fork_example.c
writer: writer.c
reader: reader.c

.PHONY: all clean clean-fifo distclean
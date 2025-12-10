CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -pthread
TARGET = reader_writer

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
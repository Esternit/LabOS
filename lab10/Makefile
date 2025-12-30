CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -pthread
TARGET = rwlock_example
SRC = rwlock_example.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
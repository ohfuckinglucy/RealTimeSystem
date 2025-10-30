CC = gcc
CFLAGS = -Wall -Wextra -std=c99

.PHONY: all clean

all: read_input poll_inputs

read_input: src/read_input.c
	$(CC) $(CFLAGS) -o $@ $^

poll_inputs: src/poll_inputs.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f read_input poll_inputs

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -I./src
LDFLAGS = -lrt -lm

.PHONY: all clean

all: jitter_benchmark

jitter_benchmark: src/jitter_benchmark.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f jitter_benchmark

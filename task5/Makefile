CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src
LDFLAGS = -lrt

.PHONY: all clean

all: task1_latency task2_mlock task3_benchmark

task1_latency: src/task1_latency.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

task2_mlock: src/task2_mlock.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

task3_benchmark: src/task3_benchmark.c src/mempool.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f task1_latency task2_mlock task3_benchmark

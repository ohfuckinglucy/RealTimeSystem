CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src
LDFLAGS = -lrt -lpthread

.PHONY: all clean

all: traffic_controller

traffic_controller: src/traffic_controller.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f traffic_controller

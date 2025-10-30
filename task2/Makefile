UNAME_S := $(shell uname -s)
BIN_DIR := bin
SRC_DIR := src

SOURCES := $(wildcard $(SRC_DIR)/*.c)
TARGETS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%,$(SOURCES))

TARGETS := $(filter-out $(BIN_DIR)/calctime1, $(TARGETS))

CFLAGS  := -O2 -g -Wall -Wextra -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS := -pthread -lm

ifeq ($(UNAME_S),Linux)
  LDFLAGS += -lrt
endif

.PHONY: all clean

all: $(TARGETS)

$(BIN_DIR)/%: $(SRC_DIR)/%.c
ifeq ($(UNAME_S),Linux)
	@mkdir -p $(BIN_DIR)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
else
	@mkdir -p $(BIN_DIR)
	@echo '#!/bin/sh' > $@
	@echo 'echo "This example is intended for Linux and was not built on $(UNAME_S)."' >> $@
	@chmod +x $@
endif

clean:
	@echo "Cleaning up..."
	@rm -rf $(BIN_DIR)


# Makefile for tasks/task1

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2 -pthread
LDFLAGS ?=
LDLIBS  ?=

BIN_DIR := bin

INTRO_SRC := src/intro
SHARED_SRC := src/shared_mem
INTR_SRC := src/interrupt
PRIO_SRC := src/inv_prio
RESMGR_SRC := src/resource_manager

# Binaries to build by default
BINS := \
	$(BIN_DIR)/hello \
	$(BIN_DIR)/intro \
	$(BIN_DIR)/nomutex \
	$(BIN_DIR)/semex \
	$(BIN_DIR)/condvar \
	$(BIN_DIR)/prodcons \
	$(BIN_DIR)/intsimple \
	$(BIN_DIR)/int \
	$(BIN_DIR)/inv_s1 \
	$(BIN_DIR)/resmgr \
	$(BIN_DIR)/resmgr_client

.PHONY: all clean test inv_s2

all: $(BIN_DIR) $(BINS)
	@echo "Build complete. Binaries are in $(BIN_DIR)/"

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# intro
$(BIN_DIR)/hello: $(INTRO_SRC)/hello.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(BIN_DIR)/intro: $(INTRO_SRC)/intro.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

# shared_mem (exclude mutex.c – for students)
$(BIN_DIR)/nomutex: $(SHARED_SRC)/nomutex.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(BIN_DIR)/semex: $(SHARED_SRC)/semex.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(BIN_DIR)/condvar: $(SHARED_SRC)/condvar.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(BIN_DIR)/prodcons: $(SHARED_SRC)/prodcons.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

# interrupt
$(BIN_DIR)/intsimple: $(INTR_SRC)/intsimple.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(BIN_DIR)/int: $(INTR_SRC)/int.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

# inv_prio
$(BIN_DIR)/inv_s1: $(PRIO_SRC)/working.c $(PRIO_SRC)/scenario_1.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

# optional: build when scenario_2 is completed by students
inv_s2: $(BIN_DIR)/inv_s2

$(BIN_DIR)/inv_s2: $(PRIO_SRC)/working.c $(PRIO_SRC)/scenario_2.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

# resource manager
$(BIN_DIR)/resmgr: $(RESMGR_SRC)/resmgr.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(BIN_DIR)/resmgr_client: $(RESMGR_SRC)/client.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(BIN_DIR)

# rudimentary test runner
test: all
	./tests/run_all.sh

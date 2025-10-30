CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -g
# -lrt для POSIX IPC (очереди, общая память)
# -pthread для POSIX семафоров
LDFLAGS := -lrt -pthread

SRC_DIR := src
BIN_DIR := bin
$(shell mkdir -p $(BIN_DIR))

SOURCES := $(wildcard $(SRC_DIR)/*.c)
TARGETS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%,$(SOURCES))

all: $(TARGETS)
	@echo "Сборка всех целей завершена."

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	@echo "Компиляция $< -> $@"
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Очистка
clean:
	@echo "Очистка бинарных файлов и временных объектов..."
	rm -rf $(BIN_DIR)
	# Дополнительная очистка системных объектов IPC, которые могли остаться
	# (может потребовать sudo, если создавались от рута)
	rm -f /dev/shm/shm_example
	rm -f /dev/shm/sem.sem_consumer_ex
	rm -f /dev/shm/sem.sem_producer_ex
	rm -f /dev/mqueue/mq_client_ex
	rm -f /dev/mqueue/mq_server_ex


.PHONY: all clean

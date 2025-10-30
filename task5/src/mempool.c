#include "mempool.h"
#include <stdlib.h>
#include <sys/mman.h>

// Узел в связном списке свободных блоков
typedef struct Node {
    struct Node* next;
} Node;

// Структура, описывающая пул
struct MemoryPool {
    size_t block_size;
    Node* free_list_head; 
    void* memory_start;    
    size_t memory_total_size;
};

MemoryPool* pool_create(size_t block_size, size_t block_count) {
    // Размер блока должен быть достаточным, чтобы вместить указатель Node
    if (block_size < sizeof(Node)) {
        block_size = sizeof(Node);
    }

    // Выделить память для самой структуры пула
    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) return NULL;

    pool->block_size = block_size;
    pool->memory_total_size = block_size * block_count;

    // Выделить один большой кусок памяти для всех блоков
    pool->memory_start = malloc(pool->memory_total_size);
    if (!pool->memory_start) {
        free(pool);
        return NULL;
    }

    // Заблокировать выделенную память в RAM
    mlock(pool->memory_start, pool->memory_total_size);

    // Разметить память как связный список свободных блоков
    pool->free_list_head = NULL;
    for (size_t i = 0; i < block_count; ++i) {
        Node* current_node = (Node*)((char*)pool->memory_start + i * block_size);
        current_node->next = pool->free_list_head;
        pool->free_list_head = current_node;
    }

    return pool;
}

void* pool_alloc(MemoryPool* pool) {
    // Извлечь первый свободный блок из списка
    if (!pool || !pool->free_list_head) {
        return NULL;
    }
    Node* block_to_alloc = pool->free_list_head;
    pool->free_list_head = block_to_alloc->next;
    return (void*)block_to_alloc;
}

void pool_free(MemoryPool* pool, void* block) {
    if (!pool || !block) return;

    // Вернуть блок в начало списка свободных блоков
    Node* node_to_free = (Node*)block;
    node_to_free->next = pool->free_list_head;
    pool->free_list_head = node_to_free;
}

void pool_destroy(MemoryPool* pool) {
    if (!pool) return;
    // Разблокировать и освободить всю память
    munlock(pool->memory_start, pool->memory_total_size);
    free(pool->memory_start);
    free(pool);
}

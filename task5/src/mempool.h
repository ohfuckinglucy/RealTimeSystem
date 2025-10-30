#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>

typedef struct MemoryPool MemoryPool;

/**
 * @brief Создает пул памяти.
 * 
 * @param block_size Размер одного блока в байтах.
 * @param block_count Количество блоков в пуле.
 * @return Указатель на созданный пул или NULL в случае ошибки.
 */
MemoryPool* pool_create(size_t block_size, size_t block_count);

/**
 * @brief Выделяет один блок из пула.
 * 
 * @param pool Указатель на пул.
 * @return Указатель на выделенный блок или NULL, если свободных блоков нет.
 */
void* pool_alloc(MemoryPool* pool);

/**
 * @brief Возвращает блок обратно в пул.
 * 
 * @param pool Указатель на пул.
 * @param block Указатель на блок, который нужно освободить.
 */
void pool_free(MemoryPool* pool, void* block);

/**
 * @brief Уничтожает пул и освобождает всю выделенную под него память.
 * 
 * @param pool Указатель на пул.
 */
void pool_destroy(MemoryPool* pool);

#endif // MEMPOOL_H

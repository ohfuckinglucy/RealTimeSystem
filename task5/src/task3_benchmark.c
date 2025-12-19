#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include "mempool.h"

#define BENCH_ITERATIONS 1000000
#define BLOCK_SIZE 128

long long timespec_diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

void benchmark_malloc() {
    printf("Benchmarking malloc/free...\n");
    struct timespec start, end;
    long long max_latency = 0;
    void* ptrs[BENCH_ITERATIONS];

    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        ptrs[i] = malloc(BLOCK_SIZE);
        clock_gettime(CLOCK_MONOTONIC, &end);
        long long latency = timespec_diff_ns(start, end);
        if (latency > max_latency) max_latency = latency;
    }

    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        free(ptrs[i]);
    }

    printf("malloc/free max latency: %lld ns\n", max_latency);
}

void benchmark_mempool() {
    printf("Benchmarking memory pool...\n");
    struct timespec start, end;
    long long max_latency = 0;
    void* ptrs[BENCH_ITERATIONS];

    // Создать пул с достаточным количеством блоков
    MemoryPool* pool = pool_create(BLOCK_SIZE, BENCH_ITERATIONS);
    if (!pool) {
        printf("Failed to create memory pool\n");
        return;
    }

    // Провести бенчмарк для pool_alloc
    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        ptrs[i] = pool_alloc(pool);
        clock_gettime(CLOCK_MONOTONIC, &end);
        long long latency = timespec_diff_ns(start, end);
        if (latency > max_latency) max_latency = latency;
    }

    // Освободить блоки
    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        pool_free(pool, ptrs[i]);
    }

    printf("pool_alloc max latency: %lld ns\n", max_latency);

    // Уничтожить пул
    pool_destroy(pool);
}

int main() {
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("mlockall failed. Try with sudo");
        return 1;
    }

    benchmark_malloc();
    printf("\n");
    benchmark_mempool();

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/mman.h>

#define ARRAY_SIZE (512 * 1024 * 1024) // 512 MB
#define PAGE_SIZE 4096
#define NUM_ITERATIONS 1000

long long timespec_diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

int main() {
    printf("Task 2: Preventing Page Faults with mlockall\n");

    // Заблокировать текущую и будущую память процесса в RAM
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("mlockall failed. Try running with sudo.");
        return 1;
    }

    char *array = (char *)malloc(ARRAY_SIZE);
    if (!array) {
        perror("malloc failed");
        return 1;
    }

    // "Прогреть" память, чтобы вызвать все minor faults на этапе инициализации
    printf("Pre-faulting memory...\n");
    for (size_t i = 0; i < ARRAY_SIZE; i += PAGE_SIZE) {
        array[i] = 0;
    }
    printf("Memory pre-faulting complete.\n");

    struct timespec start_time, end_time;
    struct rusage usage_before, usage_after;

    printf("Iter\tLatency (ns)\tMinor Faults\tMajor Faults\n");

    // Сбрасываем статистику перед основным циклом
    getrusage(RUSAGE_SELF, &usage_before);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        int index = (i * PAGE_SIZE) % ARRAY_SIZE;
        array[index] = 1;

        clock_gettime(CLOCK_MONOTONIC, &end_time);

        long long latency = timespec_diff_ns(start_time, end_time);

        // Замеряем общее количество отказов после цикла
        // В идеале, оно не должно меняться внутри цикла
        getrusage(RUSAGE_SELF, &usage_after);
        long minor_faults = usage_after.ru_minflt - usage_before.ru_minflt;
        long major_faults = usage_after.ru_majflt - usage_before.ru_majflt;

        printf("%d\t%lld\t\t%ld\t\t%ld\n", i, latency, minor_faults, major_faults);
        usage_before = usage_after; // Обновляем для следующей итерации
    }

    free(array);
    // munlockall() вызывается неявно при завершении процесса
    return 0;
}

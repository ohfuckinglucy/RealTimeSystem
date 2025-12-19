#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>

#define ARRAY_SIZE (512 * 1024 * 1024) // 512 MB
#define PAGE_SIZE 4096
#define NUM_ITERATIONS 1000

long long timespec_diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

int main() {
    printf("Task 1: Demonstrating Page Faults\n");

    // Выделить большой массив с помощью malloc
    char *array = (char *)malloc(ARRAY_SIZE);
    if (!array) {
        perror("malloc failed");
        return 1;
    }

    struct timespec start_time, end_time;
    struct rusage usage_before, usage_after;

    printf("Iter\tLatency (ns)\tMinor Faults\tMajor Faults\n");

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Получить статистику использования ресурсов ДО доступа к памяти (getrusage)
        getrusage(RUSAGE_SELF, &usage_before);

        // Замерить время ДО доступа
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        // Обратиться к элементу массива с шагом, равным размеру страницы
        // Это спровоцирует page fault, если страница еще не в памяти
        int index = (i * PAGE_SIZE) % ARRAY_SIZE;
        array[index] = 1;

        // Замерить время ПОСЛЕ доступа
        clock_gettime(CLOCK_MONOTONIC, &end_time);

        // Получить статистику использования ресурсов ПОСЛЕ доступа
        getrusage(RUSAGE_SELF, &usage_after);

        long long latency = timespec_diff_ns(start_time, end_time);
        long minor_faults = usage_after.ru_minflt - usage_before.ru_minflt;
        long major_faults = usage_after.ru_majflt - usage_before.ru_majflt;

        printf("%d\t%lld\t\t%ld\t\t%ld\n", i, latency, minor_faults, major_faults);
    }

    // for (int i = 0; i < NUM_ITERATIONS; ++i) {
    //     // Получить статистику использования ресурсов ДО доступа к памяти (getrusage)
    //     getrusage(RUSAGE_SELF, &usage_before);

    //     // Замерить время ДО доступа
    //     clock_gettime(CLOCK_MONOTONIC, &start_time);

    //     // Обратиться к элементу массива с шагом, равным размеру страницы
    //     // Это спровоцирует page fault, если страница еще не в памяти
    //     int index = (i * PAGE_SIZE) % ARRAY_SIZE;
    //     array[index] = 1;

    //     // Замерить время ПОСЛЕ доступа
    //     clock_gettime(CLOCK_MONOTONIC, &end_time);

    //     // Получить статистику использования ресурсов ПОСЛЕ доступа
    //     getrusage(RUSAGE_SELF, &usage_after);

    //     long long latency = timespec_diff_ns(start_time, end_time);
    //     long minor_faults = usage_after.ru_minflt - usage_before.ru_minflt;
    //     long major_faults = usage_after.ru_majflt - usage_before.ru_majflt;

    //     printf("%d\t%lld\t\t%ld\t\t%ld\n", i, latency, minor_faults, major_faults);
    // }

    free(array);
    return 0;
}

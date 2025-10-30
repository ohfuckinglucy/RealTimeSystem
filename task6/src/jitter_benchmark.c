#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <math.h>

#define NUM_ITERATIONS 1000

long long timespec_diff_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

void work_function() {
    double result = 0.0;
    for (int i = 0; i < 100000; ++i) {
        result += sin(i) * cos(i);
    }
}

int main(int argc, char *argv[]) {
    int target_cpu = -1;
    if (argc > 1) {
        target_cpu = atoi(argv[1]);
        printf("Target CPU specified: %d\n", target_cpu);
    }

    /* --- ЗАДАНИЕ 2: УСТАНОВКА CPU AFFINITY --- */
    if (target_cpu != -1) {
        // Создать и инициализировать маску CPU
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(target_cpu, &mask);

        // Привязать текущий процесс к ядру, указанному в маске
        if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
            perror("sched_setaffinity failed. Try with sudo.");
            return 1;
        }
        printf("Process pinned to CPU %d\n", target_cpu);
    }

    /* --- ЗАДАНИЕ 1: УСТАНОВКА REAL-TIME ПРИОРИТЕТА --- */
    struct sched_param sp;
    sp.sched_priority = 50; // Приоритет от 1 до 99 для SCHED_FIFO
    // Установить политику планирования SCHED_FIFO для текущего процесса
    if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {
        perror("sched_setscheduler failed. Try with sudo.");
        return 1;
    }
    printf("Scheduler policy set to SCHED_FIFO with priority %d\n", sp.sched_priority);

    long long latencies[NUM_ITERATIONS];
    long long min_latency = -1, max_latency = 0, total_latency = 0;

    printf("Starting benchmark...\n");
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        work_function();
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        latencies[i] = timespec_diff_ns(start, end);

        if (min_latency == -1 || latencies[i] < min_latency) {
            min_latency = latencies[i];
        }
        if (latencies[i] > max_latency) {
            max_latency = latencies[i];
        }
        total_latency += latencies[i];
    }

    double avg_latency = (double)total_latency / NUM_ITERATIONS;
    long long jitter = max_latency - min_latency;

    printf("\n--- Benchmark Results ---\n");
    printf("Min latency:    %lld ns\n", min_latency);
    printf("Max latency:    %lld ns\n", max_latency);
    printf("Avg latency:    %.2f ns\n", avg_latency);
    printf("Jitter (max-min): %lld ns\n", jitter);

    return 0;
}

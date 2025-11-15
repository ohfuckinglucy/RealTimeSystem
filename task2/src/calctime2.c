/*
 *  POSIX clock demo with 2 ms period sampling for Linux.
 *
 *  Цели:
 *  - Показать использование CLOCK_MONOTONIC и clock_getres
 *  - Реализовать периодическую выборку с шагом 2 мс через
 *    абсолютный clock_nanosleep(TIMER_ABSTIME)
 *  - Измерить фактические дельты между сэмплами и вывести статистику
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BILLION 1000000000LL
#define MILLION 1000000LL
#define NUM_SAMPLES 5000 /* 5000 * 2 ms ≈ 10 секунд эксперимента */

static inline int64_t timespec_to_ns(const struct timespec *ts) {
    return (int64_t)ts->tv_sec * BILLION + (int64_t)ts->tv_nsec;
}

static inline void ns_to_timespec(int64_t ns, struct timespec *ts) {
    ts->tv_sec = (time_t)(ns / BILLION);
    ts->tv_nsec = (long)(ns % BILLION);
}

#ifdef __linux__
int main(void) {
    struct timespec res_rt = {0}, res_mono = {0};
    struct timespec t_next = {0}, now = {0};
    const int64_t period_ns = 2 * MILLION; /* 2 ms */
    int64_t deltas_ns[NUM_SAMPLES];
    int samples = 0;

    setvbuf(stdout, NULL, _IOLBF, 0);

    if (clock_getres(CLOCK_REALTIME, &res_rt) != 0) {
        fprintf(stderr, "clock_getres(CLOCK_REALTIME) failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (clock_getres(CLOCK_MONOTONIC, &res_mono) != 0) {
        fprintf(stderr, "clock_getres(CLOCK_MONOTONIC) failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Resolution: REALTIME=%ld ns, MONOTONIC=%ld ns\n",
           (long)res_rt.tv_nsec, (long)res_mono.tv_nsec);

    if (clock_gettime(CLOCK_MONOTONIC, &t_next) != 0) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct timespec prev = t_next;

    int64_t next_ns = timespec_to_ns(&t_next) + period_ns; /* стартуем через один период */

    for (samples = 0; samples < NUM_SAMPLES; ++samples) {
        ns_to_timespec(next_ns, &t_next);

        /* Абсолютный сон до t_next: устойчив к дрейфу */
        int rc;
        do {
            rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, NULL);
        } while (rc == EINTR);
        if (rc != 0) {
            fprintf(stderr, "clock_nanosleep failed: %s\n", strerror(rc));
            return EXIT_FAILURE;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        int64_t prev_ns = timespec_to_ns(&prev);
        int64_t now_ns = timespec_to_ns(&now);
        deltas_ns[samples] = now_ns - prev_ns; /* фактическая дельта между сэмплами */

        if (samples < 10){
            printf("prev %ld.%09ld, new %ld.%09ld, delta %" PRId64 " ns\n",
               prev.tv_sec, prev.tv_nsec,
               now.tv_sec, now.tv_nsec,
               now_ns - prev_ns);
        }

        prev_ns = now_ns;
        prev = now;
        next_ns += period_ns;
    }

    /* Статистика */
    /*
    * Использование TIMER_ABSTIME с clock_nanosleep позволяет задавать
    * абсолютное время пробуждения (в монотонной шкале), а не относительную
    * задержку. Это предотвращает накопление ошибки (дрейф): даже если
    * одна итерация проспит на 10 мкс дольше, следующая всё равно
    * проснётся в точно заданное абсолютное время по расписанию.
    * Без TIMER_ABSTIME (т.е. с относительным sleep) ошибка складывалась бы
    * от итерации к итерации, и период со временем "уходил бы".
    */
    int64_t min_ns = INT64_MAX, max_ns = INT64_MIN, sum_ns = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        if (deltas_ns[i] < min_ns) min_ns = deltas_ns[i];
        if (deltas_ns[i] > max_ns) max_ns = deltas_ns[i];
        sum_ns += deltas_ns[i];
    }
    double avg_ns = (double)sum_ns / (double)NUM_SAMPLES;

    double sum_sq_diff = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        sum_sq_diff += pow((double)deltas_ns[i] - avg_ns, 2);
    }
    double std_dev_ns = sqrt(sum_sq_diff / NUM_SAMPLES);

    printf("\n=== Period statistics over %d samples (target = %" PRId64 " ns) ===\n",
        NUM_SAMPLES, period_ns);
    printf("min = %" PRId64 " ns, avg = %.1f ns, max = %" PRId64 " ns, std_dev = %.1f ns\n",
        min_ns, avg_ns, max_ns, std_dev_ns);

    return EXIT_SUCCESS;
}
#else
int main(void) {
    struct timespec res_rt = {0};
    const long period_ns = 2 * 1000000L; /* 2 ms */
    struct timespec req;
    struct timespec start, prev, now;
    const int num_samples = 5000;
    long min_ns = 999999999L, max_ns = 0;
    long long sum_ns = 0;

    setvbuf(stdout, NULL, _IOLBF, 0);
    clock_getres(CLOCK_REALTIME, &res_rt);
    printf("Resolution (CLOCK_REALTIME) ~ %ld ns (emulated periodic sleep)\n", res_rt.tv_nsec);
    clock_gettime(CLOCK_REALTIME, &start);
    prev = start;

    for (int i = 0; i < num_samples; ++i) {
        req.tv_sec = 0;
        req.tv_nsec = period_ns;
        nanosleep(&req, NULL);
        clock_gettime(CLOCK_REALTIME, &now);
        long delta = (long)((now.tv_sec - prev.tv_sec) * 1000000000LL + (now.tv_nsec - prev.tv_nsec));
        if (delta < min_ns) min_ns = delta;
        if (delta > max_ns) max_ns = delta;
        sum_ns += delta;
        prev = now;
    }
    double avg = (double)sum_ns / (double)num_samples;
    printf("2ms-period stats over %d samples (relative_sleep): min=%ld ns, avg=%.1f ns, max=%ld ns\n",
           num_samples, min_ns, avg, max_ns);
    return EXIT_SUCCESS;
}
#endif
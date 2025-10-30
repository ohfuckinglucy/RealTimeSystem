/*
 * Демонстрация периодического таймера с использованием timerfd.
 *
 * Цель: Показать способ создания периодических задач. timerfd представляет таймер как файловый
 * дескриптор, что позволяет интегрировать его с механизмами вроде poll/epoll.
 *
 * Сценарий:
 * 1. Создать таймер с помощью timerfd_create.
 * 2. Настроить его на первое срабатывание через 5 секунд,
 *    а затем периодически каждые 1.5 секунды.
 * 3. В цикле ожидать срабатывания таймера, читая из его файлового дескриптора.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/timerfd.h>
#endif

static inline int64_t to_ns(const struct timespec *ts) {
    return (int64_t)ts->tv_sec * 1000000000LL + (int64_t)ts->tv_nsec;
}

#ifdef __linux__
int main(void) {
    int tfd;
    struct itimerspec its;
    struct timespec now;
    uint64_t expirations;
    const int iterations_to_run = 5;

    setvbuf(stdout, NULL, _IOLBF, 0);

    // CLOCK_MONOTONIC - лучший выбор для таймеров, т.к. на него не влияет
    // изменение системного времени (NTP, date).
    // TFD_CLOEXEC - хорошая практика, чтобы дескриптор не наследовался дочерними процессами.
    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (tfd == -1) {
        perror("timerfd_create failed");
        return EXIT_FAILURE;
    }

    // Настройка времени: первое срабатывание
    its.it_value.tv_sec = 5;
    its.it_value.tv_nsec = 0;

    // Настройка времени: период после первого срабатывания
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 500000000; // 1.5 секунды

    if (timerfd_settime(tfd, 0, &its, NULL) == -1) {
        perror("timerfd_settime failed");
        close(tfd);
        return EXIT_FAILURE;
    }

    printf("Timer configured. Waiting for %d expirations...\n", iterations_to_run);
    for (int i = 0; i < iterations_to_run; ++i) {
        // read() блокируется, пока таймер не сработает.
        // Возвращает 8 байт (uint64_t), содержащих количество истечений таймера.
        // Если система не сильно нагружена, это значение будет 1.
        ssize_t rd = read(tfd, &expirations, sizeof(expirations));
        if (rd < 0) {
            if (errno == EINTR) { // Прервано сигналом, можно проигнорировать
                --i;
                continue;
            }
            perror("read(timerfd) failed");
            close(tfd);
            return EXIT_FAILURE;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("clock_gettime failed");
            close(tfd);
            return EXIT_FAILURE;
        }
        printf("Timer expired! Timestamp: [%ld.%09ld], expirations counter: %" PRIu64 "\n",
               now.tv_sec, now.tv_nsec, expirations);
    }

    close(tfd);
    return EXIT_SUCCESS;
}
#else
int main(void) {
    printf("reptimer_timerfd: Linux-only example (timerfd not available on this platform)\n");
    return 0;
}
#endif


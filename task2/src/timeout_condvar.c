/*
 * Демонстрация ожидания на условной переменной с таймаутом (pthread_cond_timedwait).
 *
 * Цель: Показать, как один поток может ждать события от другого,
 * но не бесконечно, а в течение ограниченного времени. Это базовый
 * паттерн для межпоточной синхронизации в СРВ.
 *
 * Сценарий:
 * 1. Потребитель (main) пытается ждать на условной переменной, но с таймаутом в 100 мс.
 *    Так как производитель еще не запущен, ожидание завершается по таймауту.
 * 2. Запускается поток-производитель, который засыпает на 200 мс, а затем
 *    сигнализирует условную переменную.
 * 3. Потребитель снова ждет, но уже с таймаутом в 1 секунду. Он должен
 *    успешно дождаться сигнала от производителя.
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int event_ready = 0;

// Вспомогательная функция для расчета абсолютного времени в будущем.
// pthread_cond_timedwait требует именно АБСОЛЮТНОЕ время.
static void abs_time_after_ms(struct timespec *ts, int ms) {
    // Важно использовать CLOCK_REALTIME, так как этого требует стандарт POSIX
    // для pthread_cond_timedwait.
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += ms / 1000;
    long add_ns = (long)(ms % 1000) * 1000000L;
    ts->tv_nsec += add_ns;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

// Поток-производитель
static void *producer(void *arg) {
    (void)arg;
    printf("[PRODUCER] sleeping for 200 ms...\n");
    usleep(200 * 1000); /* 200 ms */

    printf("[PRODUCER] signaling condition...\n");
    pthread_mutex_lock(&mtx);
    event_ready = 1;
    // pthread_cond_signal будит ОДИН из ждущих потоков.
    // Для пробуждения всех используется pthread_cond_broadcast.
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    // --- Сценарий 1: Ожидание завершается по таймауту ---
    printf("[CONSUMER] Doing a timed wait of 100ms, expecting a timeout...\n");
    struct timespec ts;
    pthread_mutex_lock(&mtx);
    abs_time_after_ms(&ts, 100);

    // pthread_cond_timedwait атомарно разблокирует мьютекс и начинает ждать.
    // Перед возвратом мьютекс снова блокируется.
    int rc = pthread_cond_timedwait(&cv, &mtx, &ts);
    if (rc == ETIMEDOUT) {
        printf("[CONSUMER] Timed out as expected.\n");
    } else {
        printf("[CONSUMER] Unexpected success or error: %s\n", strerror(rc));
    }
    pthread_mutex_unlock(&mtx);

    // --- Сценарий 2: Успешное ожидание ---
    printf("\n[CONSUMER] Starting producer and waiting up to 1000ms...\n");
    pthread_t th;
    if (pthread_create(&th, NULL, producer, NULL) != 0) {
        perror("pthread_create failed");
        return EXIT_FAILURE;
    }

    pthread_mutex_lock(&mtx);
    abs_time_after_ms(&ts, 1000);
    // Цикл while необходим для обработки "ложных пробуждений" (spurious wakeups),
    // когда поток просыпается без реального сигнала.
    while (!event_ready && rc != ETIMEDOUT) {
        rc = pthread_cond_timedwait(&cv, &mtx, &ts);
    }

    if (event_ready) {
        printf("[CONSUMER] Condition was signaled successfully.\n");
    } else if (rc == ETIMEDOUT) {
        printf("[CONSUMER] Unexpected timeout.\n");
    } else {
        printf("[CONSUMER] Error during wait: %s\n", strerror(rc));
    }
    pthread_mutex_unlock(&mtx);

    pthread_join(th, NULL);
    return 0;
}



/*
 * Демонстрация ожидания на очереди POSIX MQ с таймаутом (mq_timedreceive).
 *
 * Цель: Показать, как организовать межпроцессное или межпоточное взаимодействие
 * с ограниченным временем ожидания, используя стандартный механизм очередей POSIX.
 *
 * Сценарий:
 * 1. Потребитель (main) пытается прочитать из пустой очереди с таймаутом в 100 мс.
 *    Ожидание завершается по таймауту.
 * 2. Запускается поток-отправитель, который через 300 мс посылает сообщение в очередь.
 * 3. Потребитель снова пытается прочитать, но с таймаутом 1 секунда,
 *    и должен успешно получить сообщение.
 */

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Имя очереди должно начинаться со слэша.
static const char *QNAME = "/rt_timeout_demo_mq";

// Вспомогательная функция для расчета абсолютного времени в будущем.
// mq_timedreceive, как и pthread_cond_timedwait, требует АБСОЛЮТНОЕ время.
static void abs_time_after_ms(struct timespec *ts, int ms) {
    // Важно использовать CLOCK_REALTIME, так как этого требует стандарт POSIX.
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += ms / 1000;
    long add_ns = (long)(ms % 1000) * 1000000L;
    ts->tv_nsec += add_ns;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

// Поток-отправитель
static void *sender(void *arg) {
    mqd_t mq = *(mqd_t *)arg;
    const char *msg = "hello";
    printf("[SENDER] sleeping for 300ms...\n");
    usleep(300 * 1000); /* 300 ms */
    printf("[SENDER] sending message...\n");
    if (mq_send(mq, msg, strlen(msg) + 1, 0) != 0) {
        perror("mq_send failed");
    }
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Атрибуты очереди: максимальное кол-во сообщений и их макс. размер.
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 128;

    // O_CLOEXEC - хорошая практика, чтобы дескриптор не наследовался.
    mqd_t mq = mq_open(QNAME, O_CREAT | O_RDWR | O_CLOEXEC, 0600, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open failed");
        return EXIT_FAILURE;
    }

    // --- Сценарий 1: Таймаут ---
    printf("[RECEIVER] Waiting for 100ms on an empty queue, expecting timeout...\n");
    struct timespec ts;
    abs_time_after_ms(&ts, 100);
    char buf[128];
    ssize_t n = mq_timedreceive(mq, buf, sizeof(buf), NULL, &ts);
    if (n < 0 && errno == ETIMEDOUT) {
        printf("[RECEIVER] Timed out as expected.\n");
    } else if (n >= 0) {
        printf("[RECEIVER] Unexpectedly received message: %s\n", buf);
    } else {
        perror("[RECEIVER] mq_timedreceive error");
    }

    // --- Сценарий 2: Успешное получение ---
    printf("\n[RECEIVER] Starting sender and waiting for up to 1000ms...\n");
    pthread_t th;
    if (pthread_create(&th, NULL, sender, &mq) != 0) {
        perror("pthread_create failed");
        mq_close(mq);
        mq_unlink(QNAME);
        return EXIT_FAILURE;
    }

    abs_time_after_ms(&ts, 1000);
    n = mq_timedreceive(mq, buf, sizeof(buf), NULL, &ts);
    if (n >= 0) {
        printf("[RECEIVER] Successfully received '%s' within timeout.\n", buf);
    } else if (errno == ETIMEDOUT) {
        printf("[RECEIVER] Unexpected timeout.\n");
    } else {
        perror("[RECEIVER] mq_timedreceive error");
    }

    pthread_join(th, NULL);
    // Важно закрывать и удалять очередь, чтобы она не осталась в системе.
    mq_close(mq);
    mq_unlink(QNAME);
    return 0;
}
#else
#include <stdio.h>
int main(void) {
    printf("timeout_mq: Linux-only example (POSIX MQ not available)\n");
    return 0;
}
#endif



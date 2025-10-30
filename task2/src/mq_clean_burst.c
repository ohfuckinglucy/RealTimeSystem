/*
 * Демонстрация "осушения" очереди сообщений POSIX MQ.
 *
 * Сценарий:
 * В системах реального времени часто возникает ситуация, когда после
 * обработки одного события (например, прерывания) в очереди оказывается
 * сразу несколько однотипных сообщений ("burst"). Чтобы обработать текущее
 * состояние системы, а не устаревшие события, применяется паттерн "осушения":
 * 1. Блокирующе дождаться первого сообщения.
 * 2. Обработать его.
 * 3. Переключить очередь в НЕБЛОКИРУЮЩИЙ режим.
 * 4. В цикле читать все оставшиеся сообщения, пока очередь не опустеет
 *    (mq_receive вернет EAGAIN).
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

static const char *QNAME = "/rt_burst_demo";

static void *producer(void *arg) {
    mqd_t mq = *(mqd_t *)arg;
    char msg[32];
    for (int i = 0; i < 7; ++i) {
        snprintf(msg, sizeof(msg), "msg-%d", i);
        if (mq_send(mq, msg, strlen(msg) + 1, 0) != 0) {
            fprintf(stderr, "producer: mq_send failed: %s\n", strerror(errno));
        }
    }
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 32;
    attr.mq_msgsize = 64;

    mqd_t mq = mq_open(QNAME, O_CREAT | O_RDWR | O_CLOEXEC, 0600, &attr);
    if (mq == (mqd_t)-1) {
        // Используем perror для более информативного вывода ошибок
        perror("mq_open failed");
        return EXIT_FAILURE;
    }

    pthread_t th;
    if (pthread_create(&th, NULL, producer, &mq) != 0) {
        perror("pthread_create failed");
        mq_close(mq);
        mq_unlink(QNAME);
        return EXIT_FAILURE;
    }

    /* 1. Блокируемся в ожидании первого сообщения */
    char buf[64];
    printf("Waiting for the first message...\n");
    ssize_t n = mq_receive(mq, buf, sizeof(buf), NULL);
    if (n >= 0) {
        printf("Received first message: '%s'\n", buf);
    } else {
        perror("mq_receive failed");
    }

    /* 2. Переключаем очередь в неблокирующий режим для "осушения" */
    struct mq_attr cur;
    mq_getattr(mq, &cur);
    cur.mq_flags |= O_NONBLOCK;
    mq_setattr(mq, &cur, NULL);

    printf("Draining the rest of the burst: ");
    int drained_count = 0;
    while (1) {
        n = mq_receive(mq, buf, sizeof(buf), NULL);
        if (n >= 0) {
            printf("'%s' ", buf);
            drained_count++;
        } else if (errno == EAGAIN) {
            // EAGAIN означает "попробуйте снова", в неблокирующем режиме это
            // индикатор того, что очередь пуста.
            break; /* Успешно осушили */
        } else if (errno == EINTR) {
            // Прервано сигналом, просто повторяем попытку
            continue;
        } else {
            // Другая ошибка
            perror("\nerror during drain");
            break;
        }
    }
    printf("\nDrained %d more messages.\n", drained_count);

    pthread_join(th, NULL);
    mq_close(mq);
    mq_unlink(QNAME);
    return 0;
}
#else
#include <stdio.h>
int main(void) {
    printf("mq_clean_burst: Linux-only example (POSIX MQ not available)\n");
    return 0;
}
#endif



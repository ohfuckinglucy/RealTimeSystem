/*
 * Демонстрация ожидания на файловом дескрипторе с таймаутом (poll).
 *
 * Цель: Показать базовый механизм ожидания события ввода-вывода
 * с ограниченным временем. poll() является классическим системным вызовом
 * для мультиплексирования ввода-вывода.
 *
 * Сценарий:
 * 1. Создается pipe. Потребитель (main) пытается ждать данных из pipe
 *    с таймаутом 300 мс. Так как никто не пишет в pipe, poll() возвращает 0 (таймаут).
 * 2. Запускается поток-писатель, который засыпает на 200 мс, а затем
 *    пишет один байт в pipe.
 * 3. Потребитель снова вызывает poll(), но с таймаутом 1000 мс. Он должен
 *    успешно дождаться данных и прочитать их.
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int write_fd;
    int delay_ms;
} writer_args_t;

// Поток-писатель
static void *writer_thread(void *arg) {
    writer_args_t *args = (writer_args_t *)arg;
    struct timespec ts = {.tv_sec = args->delay_ms / 1000,
                          .tv_nsec = (args->delay_ms % 1000) * 1000000L};
    printf("[WRITER] sleeping for %d ms...\n", args->delay_ms);
    nanosleep(&ts, NULL);

    printf("[WRITER] writing data to pipe...\n");
    const char ch = 'X';
    if (write(args->write_fd, &ch, 1) != 1) {
        perror("writer: write failed");
    }
    return NULL;
}

int main(void) {
    int fds[2]; // fds[0] - read end, fds[1] - write end
    struct pollfd pfd;
    pthread_t th;
    writer_args_t wargs;
    char buf;

    setvbuf(stdout, NULL, _IOLBF, 0);

    if (pipe(fds) != 0) {
        perror("pipe failed");
        return EXIT_FAILURE;
    }

    pfd.fd = fds[0]; // Ждем на чтение из pipe
    pfd.events = POLLIN; // Интересующее событие - "есть данные для чтения"

    // --- Сценарий 1: Таймаут ---
    printf("[READER] Polling for 300ms, expecting timeout...\n");
    int rc = poll(&pfd, 1, 300); // 1 - кол-во дескрипторов, 300 - таймаут в мс

    if (rc == 0) {
        printf("[READER] poll() timed out as expected.\n");
    } else if (rc < 0) {
        perror("[READER] poll failed");
        return EXIT_FAILURE;
    } else {
        printf("[READER] poll() detected readiness unexpectedly.\n");
    }

    // --- Сценарий 2: Успешное ожидание ---
    printf("\n[READER] Starting writer and polling for 1000ms...\n");
    wargs.write_fd = fds[1];
    wargs.delay_ms = 200;
    if (pthread_create(&th, NULL, writer_thread, &wargs) != 0) {
        perror("pthread_create failed");
        return EXIT_FAILURE;
    }

    pfd.revents = 0; // Сбрасываем поле возвращенных событий
    rc = poll(&pfd, 1, 1000);

    if (rc == 1 && (pfd.revents & POLLIN)) {
        printf("[READER] poll() detected data. Reading...\n");
        if (read(fds[0], &buf, 1) == 1) {
            printf("[READER] Received byte '%c' within timeout.\n", buf);
        } else {
            perror("[READER] read failed");
        }
    } else if (rc == 0) {
        printf("[READER] poll() unexpectedly timed out.\n");
    } else if (rc < 0) {
        perror("[READER] poll failed");
    }

    pthread_join(th, NULL);
    close(fds[0]);
    close(fds[1]);
    return EXIT_SUCCESS;
}



/*
 * Демонстрация ppoll() для ожидания с атомарной разблокировкой сигналов.
 *
 * Цель: Показать, как правильно и безопасно обрабатывать сигналы во время
 * блокирующего ожидания на файловых дескрипторах. Простое использование
 * poll() или read() может привести к состоянию гонки (race condition),
 * если сигнал придет до того, как блокирующий вызов начался.
 * ppoll() решает эту проблему, атомарно изменяя маску сигналов на время ожидания.
 *
 * Сценарий:
 * 1. Основной поток блокирует сигнал SIGUSR1 с помощью pthread_sigmask().
 * 2. Создается дочерний поток, который через 1 секунду посылает SIGUSR1 основному.
 * 3. Основной поток вызывает ppoll(), передавая ему маску сигналов,
 *    в которой SIGUSR1 РАЗБЛОКИРОВАН.
 * 4. ppoll() атомарно снимает блокировку с SIGUSR1 и начинает ждать.
 * 5. Когда приходит сигнал, он прерывает ppoll() (возвращается EINTR),
 *    и обработчик сигнала выполняется немедленно.
 * 6. После возврата из ppoll() исходная маска (с заблокированным SIGUSR1)
 *    автоматически восстанавливается.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Флаг, который будет (безопасно) установлен в обработчике сигнала.
// volatile - чтобы компилятор не оптимизировал доступ к переменной.
// sig_atomic_t - тип, запись в который является атомарной.
static volatile sig_atomic_t signal_received = 0;

static void signal_handler(int signum) {
    (void)signum;
    signal_received = 1;
    // ПРЕДУПРЕЖДЕНИЕ: write() - одна из немногих async-signal-safe функций.
    // Использовать printf() и другие функции стандартной библиотеки
    // в обработчиках сигналов НЕБЕЗОПАСНО и может привести к deadlock.
    write(STDOUT_FILENO, "Signal handler executed!\n", 25);
}

// Поток, посылающий сигнал
static void *thread_sender(void *arg) {
    (void)arg;
    printf("[SENDER] sleeping for 1 second...\n");
    sleep(1);
    printf("[SENDER] sending SIGUSR1 to main thread...\n");
    // pthread_kill посылает сигнал конкретному потоку.
    pthread_kill(pthread_main_np(), SIGUSR1);
    return NULL;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    // 1. Настройка обработчика сигнала
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // 2. Блокируем SIGUSR1. Сохраняем исходную маску (original_mask),
    // чтобы передать ее в ppoll. В original_mask SIGUSR1 не заблокирован.
    sigset_t blocked_mask, original_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &blocked_mask, &original_mask) != 0) {
        perror("pthread_sigmask");
        return EXIT_FAILURE;
    }
    printf("Main thread blocked SIGUSR1.\n");

    // 3. Создаем pipe, на котором будем ждать (хотя данных там не будет)
    int fds[2];
    if (pipe(fds) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    struct pollfd pfd = {.fd = fds[0], .events = POLLIN};

    // 4. Создаем поток, который пошлет нам сигнал
    pthread_t tid;
    if (pthread_create(&tid, NULL, thread_sender, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    // 5. Вызываем ppoll().
    // Он будет ждать 5 секунд И атомарно заменит текущую маску сигналов
    // (где SIGUSR1 заблокирован) на original_mask (где он разблокирован).
    // Как только ppoll вернет управление, исходная маска будет восстановлена.
    printf("Calling ppoll() with unblocked signal mask, waiting for signal...\n");
    struct timespec timeout = {.tv_sec = 5, .tv_nsec = 0};

    // Это ключевой вызов. Передача `&original_mask` - это то,
    // что отличает ppoll от poll и решает проблему race condition.
    int rc = ppoll(&pfd, 1, &timeout, &original_mask);

    // 6. Анализ результата
    if (rc == -1) {
        if (errno == EINTR) {
            printf("ppoll was correctly interrupted by a signal.\n");
        } else {
            perror("ppoll failed");
        }
    } else if (rc == 0) {
        printf("ppoll timed out, which is unexpected.\n");
    } else {
        printf("ppoll detected data, which is unexpected.\n");
    }

    if (signal_received) {
        printf("Verified that the signal handler was executed.\n");
    } else {
        printf("Warning: signal handler was not executed.\n");
    }

    pthread_join(tid, NULL);
    close(fds[0]);
    close(fds[1]);
    return 0;
}
/*
 * Сравнение стратегий таймаутов:
 *
 * - poll() — ожидание событий на файловых дескрипторах (I/O, pipe, сокеты).
 * - pthread_cond_timedwait() — межпоточная синхронизация с ограничением по времени.
 * - mq_timedreceive() — межпроцессное взаимодействие через POSIX-очереди.
 * - ppoll() — как poll(), но с безопасной обработкой сигналов
 *              (атомарно меняет маску сигналов во время ожидания).
 *
 * poll — универсален для I/O;
 * condvar — для потоков;
 * mq — для процессов;
 * ppoll — для безопасного ожидания с сигналами.
 */

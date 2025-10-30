#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "common.h"

// Глобальные переменные
SharedData shared_data; // Разделяемые данные
timer_t timer;          // POSIX таймер
volatile sig_atomic_t timer_expired = 0; // Флаг, устанавливаемый по сигналу таймера

// Обработчик сигнала от таймера
void timer_handler(int sig) {
    timer_expired = 1;
}

// Функция для вывода текущего состояния светофоров
void print_lights(TrafficState state) {
    // TODO: Реализовать красивый вывод в консоль
    printf("State: %d | ", state);
    switch (state) {
        case STATE_NS_GREEN:    printf("NS: GREEN,  EW: RED\n"); break;
        case STATE_NS_YELLOW:   printf("NS: YELLOW, EW: RED\n"); break;
        case STATE_EW_GREEN:    printf("NS: RED,    EW: GREEN\n"); break;
        case STATE_EW_YELLOW:   printf("NS: RED,    EW: YELLOW\n"); break;
        case STATE_ALL_RED:     printf("NS: RED,    EW: RED\n"); break;
        case STATE_PED_CROSS:   printf("NS: RED,    EW: RED  | WALK\n"); break;
        case STATE_EMERGENCY:   printf("EMERGENCY! NS: RED,    EW: RED\n"); break;
        default:                printf("Unknown state\n"); break;
    }
}

// Функция потока контроллера (FSM)
void* controller_thread_func(void* arg) {
    TrafficState next_state = STATE_ALL_RED;

    while (1) {
        pthread_mutex_lock(&shared_data.mutex);
        shared_data.current_state = next_state;
        print_lights(shared_data.current_state);
        pthread_mutex_unlock(&shared_data.mutex);

        struct itimerspec its;
        timer_expired = 0;

        // Логика конечного автомата
        switch (shared_data.current_state) {
            // Реализовать логику переходов FSM
            // Пример для одного состояния:
            case STATE_NS_GREEN:
                its.it_value.tv_sec = GREEN_DURATION;
                its.it_value.tv_nsec = 0;
                next_state = STATE_NS_YELLOW;
                break;

            // ... другие состояния ...

            default:
                its.it_value.tv_sec = 1; // Безопасная задержка по умолчанию
                next_state = STATE_ALL_RED;
                break;
        }

        // Взводим таймер
        its.it_interval.tv_sec = 0; // Не повторяющийся таймер
        its.it_interval.tv_nsec = 0;
        timer_settime(timer, 0, &its, NULL);

        // Ожидаем истечения таймера
        while (!timer_expired) {
            // Здесь можно проверять асинхронные события (режим ЧС)
            usleep(10000); // Спим недолго, чтобы не грузить CPU
        }
    }
    return NULL;
}

// Функция потока для пользовательского ввода
void* input_thread_func(void* arg) {
    printf("Input keys: n (NS ped), e (EW ped), s (siren)\n");
    while (1) {
        char c = getchar();
        // Реализовать обработку нажатий и потокобезопасную установку флагов
        // в shared_data
    }
    return NULL;
}

int main() {
    // Инициализировать мьютекс
    pthread_mutex_init(&shared_data.mutex, NULL);

    // Настроить обработчик сигнала для таймера (sigaction)
    struct sigaction sa;
    sa.sa_handler = timer_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGRTMIN, &sa, NULL);

    // Создать POSIX таймер (timer_create)
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timer;
    if (timer_create(CLOCK_REALTIME, &sev, &timer) == -1) {
        perror("timer_create failed");
        return 1;
    }

    // Создать потоки контроллера и пользовательского ввода
    pthread_t controller_thread, input_thread;
    pthread_create(&controller_thread, NULL, controller_thread_func, NULL);
    // pthread_create(&input_thread, NULL, input_thread_func, NULL);

    // Дождаться завершения потоков
    pthread_join(controller_thread, NULL);
    // pthread_join(input_thread, NULL);

    // Уничтожить мьютекс и таймер
    pthread_mutex_destroy(&shared_data.mutex);
    timer_delete(timer);

    return 0;
}

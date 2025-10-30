#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>

// Состояния конечного автомата
typedef enum {
    STATE_INIT,
    STATE_NS_GREEN,     // Север-Юг зеленый, Запад-Восток красный
    STATE_NS_YELLOW,    // Север-Юг желтый, Запад-Восток красный
    STATE_EW_GREEN,     // Запад-Восток зеленый, Север-Юг красный
    STATE_EW_YELLOW,    // Запад-Восток желтый, Север-Юг красный
    STATE_ALL_RED,      // Всем красный (для безопасности)
    STATE_PED_CROSS,    // Переход для пешеходов
    STATE_EMERGENCY     // Режим ЧС
} TrafficState;

// Длительности состояний в секундах
#define GREEN_DURATION 10
#define YELLOW_DURATION 2
#define ALL_RED_DURATION 1
#define PED_CROSS_DURATION 8

// Общая структура для данных, разделяемых между потоками
typedef struct {
    pthread_mutex_t mutex;      // Мьютекс для защиты данных
    TrafficState current_state; // Текущее состояние FSM

    // Флаги запросов от потока ввода
    int ped_ns_request;         // Запрос пешехода Север-Юг
    int ped_ew_request;         // Запрос пешехода Запад-Восток
    int emergency_request;      // Запрос режима ЧС

} SharedData;

#endif // COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define LOOP_COUNT 30
#define SLEEP_TIME 1000
#define BUFSIZE 10

// Структура для примеров
typedef struct _msg_priority {
  int tid;
  int priority;
} msg_priority;

// Инициализация мьютекса ресурса.
// enable_prio_inherit = 0 — обычный мьютекс; 1 — попытка включить наследование приоритета.
int init_resource_mutex(int enable_prio_inherit);

// Имитируем работу с ресурсом (внутри критической секции)
void working(int process_id);

// Потоки для демонстрации инверсии приоритетов
void *server(void *arg);   // низкий приоритет: удерживает мьютекс ресурса
void *t1(void *arg);       // средний приоритет: CPU-bound
void *t2(void *arg);       // высокий приоритет: ждёт мьютекс
#include "working.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t resource_mutex;

int init_resource_mutex(int enable_prio_inherit)
{
  pthread_mutexattr_t attr;
  if (pthread_mutexattr_init(&attr) != 0) return -1;
#ifdef PTHREAD_PRIO_INHERIT
  if (enable_prio_inherit) {
    // Попробуем включить наследование приоритета, если система поддерживает
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
  }
#else
  (void)enable_prio_inherit;
#endif
  int rc = pthread_mutex_init(&resource_mutex, &attr);
  pthread_mutexattr_destroy(&attr);
  return rc == 0 ? 0 : -1;
}

static void busy_ms(int ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&ts, NULL);
}

void working(int tid)
{
  // Имитация работы с общим ресурсом
  for (int i = 0; i < 5; i++) {
    printf("server is working for %d - %dth start\n", tid, i);
    busy_ms(1000);
    printf("server is working for %d - %dth end\n", tid, i);
  }
  printf("server finished all work for %d\n", tid);
}

// Низкоприоритетный поток: захватывает ресурс и держит его некоторое время
void *server(void *arg)
{
  (void)arg;
  printf("[SERVER] стартует и захватывает ресурс\n");
  pthread_mutex_lock(&resource_mutex);
  // Держим ресурс достаточно долго, чтобы высокий приоритет т2 подождал
  working(0);
  pthread_mutex_unlock(&resource_mutex);
  printf("[SERVER] освободил ресурс\n");
  return NULL;
}

// Средний приоритет: просто грузит CPU/время, создавая условия для инверсии
void *t1(void *arg)
{
  (void)arg;
  printf("[T1 mid] стартует (фоновая нагрузка)\n");
  for (int i = 0; i < 200; i++) {
    // Загрузка времени
    busy_ms(10);
  }
  printf("[T1 mid] завершился\n");
  return NULL;
}

// Высокий приоритет: пытается получить ресурс
void *t2(void *arg)
{
  (void)arg;
  printf("[T2 high] пытается получить ресурс\n");
  pthread_mutex_lock(&resource_mutex);
  printf("[T2 high] получил ресурс\n");
  busy_ms(20);
  pthread_mutex_unlock(&resource_mutex);
  printf("[T2 high] освободил ресурс и завершился\n");
  return NULL;
}
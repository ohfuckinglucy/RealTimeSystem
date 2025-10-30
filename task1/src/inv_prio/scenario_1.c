#include "working.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>

static int set_thread_priority(pthread_attr_t *attr, int policy, int prio)
{
  pthread_attr_init(attr);
  pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(attr, policy);
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = prio;
  return pthread_attr_setschedparam(attr, &sp);
}

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  // Настраиваем SCHED_FIFO приоритеты: server=10 (низкий), t1=20 (средний), t2=30 (высокий)
  const int policy = SCHED_FIFO;
  const int prio_server = 10;
  const int prio_t1 = 20;
  const int prio_t2 = 30;

  // Мьютекс без наследования приоритета — демонстрация инверсии
  if (init_resource_mutex(0) != 0) {
    perror("init_resource_mutex");
    return EXIT_FAILURE;
  }

  pthread_attr_t attr_server, attr_t1, attr_t2;
  if (set_thread_priority(&attr_server, policy, prio_server) != 0) {
    perror("attr_server");
  }
  if (set_thread_priority(&attr_t1, policy, prio_t1) != 0) {
    perror("attr_t1");
  }
  if (set_thread_priority(&attr_t2, policy, prio_t2) != 0) {
    perror("attr_t2");
  }

  pthread_t th_server, th_t1, th_t2;

  // Запускаем сервер (низкий приоритет) — он захватит ресурс и будет держать
  if (pthread_create(&th_server, &attr_server, server, NULL) != 0) {
    perror("pthread_create server");
    return EXIT_FAILURE;
  }
  // Небольшая фора серверу
  usleep(50 * 1000);

  // Запускаем высокий приоритет (попытается взять мьютекс) и средний (фон)
  if (pthread_create(&th_t2, &attr_t2, t2, NULL) != 0) {
    perror("pthread_create t2");
    return EXIT_FAILURE;
  }
  if (pthread_create(&th_t1, &attr_t1, t1, NULL) != 0) {
    perror("pthread_create t1");
    return EXIT_FAILURE;
  }

  // Ждём завершения
  void *st;
  pthread_join(th_t1, &st);
  pthread_join(th_t2, &st);
  pthread_join(th_server, &st);

  printf("scenario_1: завершено. Наблюдайте временную задержку у t2 из-за t1 (инверсия приоритетов).\n");
  return EXIT_SUCCESS;
}
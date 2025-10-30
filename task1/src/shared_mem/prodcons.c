/*
 *  Демонстрация POSIX условных переменных на примере "Производитель и потребитель".
 *  Так как у нас всего два потока, ожидающих сигнала,    
 *  в любой момент работы одного из них мы можем просто использовать вызов
 *  pthread_cond_signal для пробуждения второго потока.
 *
*/

#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

// mutex и условная переменная

pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
volatile int        state = 0;    // переменная состояния
volatile int        product = 0;  // вывод производителя
void    *producer (void *);
void    *consumer (void *);
void    do_producer_work (void);
void    do_consumer_work (void);
char    *progname = "prodcons";

int main ()
{
  pthread_t cons, prod; // Создаем потоки
  setvbuf (stdout, NULL, _IOLBF, 0);
  pthread_create (&cons, NULL, consumer, NULL);
  pthread_create (&prod, NULL, producer, NULL);
  sleep (20);     // Позволим потокам выполнить "работу"
  printf ("%s:  main, exiting\n", progname);
  return 0;
}

// Производитель
void *producer (void *arg)
{
  while (1) {
    pthread_mutex_lock (&mutex);
    while (state == 1) {
      pthread_cond_wait (&cond, &mutex);
    }
    printf ("%s:  produced %d, state %d\n", progname, ++product, state);
    state = 1;
    pthread_cond_signal (&cond);
    pthread_mutex_unlock (&mutex);
    do_producer_work ();
  }
  return (NULL);
}

// Потребитель

void *consumer (void *arg)
{
  while (1) {
    pthread_mutex_lock (&mutex);
    while (state == 0) {
      pthread_cond_wait (&cond, &mutex);
    }
    printf ("%s:  consumed %d, state %d\n", progname, product, state);
    state = 0;
    pthread_cond_signal (&cond);
    pthread_mutex_unlock (&mutex);
    do_consumer_work ();
  }
  return (NULL);
}

void do_producer_work (void)
{
  usleep (100 * 1000);
}

void do_consumer_work (void)
{
  usleep (100 * 1000);
}
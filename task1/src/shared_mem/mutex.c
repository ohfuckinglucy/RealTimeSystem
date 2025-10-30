// Усовершенствуйте код nomutex.c (Листинг №3) с использованием конструкций мьютекса таким образом, чтобы
// решить проблему доступа к общей переменной.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define NumThreads      16       // позже установите значение 16

volatile int     var1;
volatile int     var2;

pthread_mutex_t mutex; // объявление мьютекста

void    *update_thread (void *);
char    *progname = "nomutex";

int main ()
{
    pthread_t           threadID [NumThreads];  // хранит ID потоков
    pthread_attr_t      attrib;                 // атрибуты планирования
    struct sched_param  param;                  // для установки приоритета
    int                 i, policy;

    pthread_mutex_init(&mutex, NULL); // Инициализация мьютекса

    setvbuf (stdout, NULL, _IOLBF, 0);
    var1 = var2 = 0;        /* инициализация известных */
    printf ("%s:  starting; creating threads\n", progname);
    /*
     *  we want to create the new threads using Round Robin
     *  scheduling, and a lowered priority, so set up a thread 
     *  attributes structure.  We use a lower priority since these
     *  threads will be hogging the CPU
    */

    pthread_getschedparam (pthread_self(), &policy, &param);
    pthread_attr_init (&attrib);
    pthread_attr_setinheritsched (&attrib, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy (&attrib, SCHED_RR);
    param.sched_priority += 2;        // Снизить приоритет на 2 уровня поменял на += 
    pthread_attr_setschedparam (&attrib, &param);

    // Создаем потоки. С каждым завершением вызова pthread_create запускается поток.
    for (i = 0; i < NumThreads; i++) {
        pthread_create (&threadID [i], &attrib, &update_thread, (void *)(long)i);
    }

    sleep (20);
    printf ("%s:  stopping; cancelling threads\n", progname);
    for (i = 0; i < NumThreads; i++) {
      pthread_cancel (threadID [i]);
    }
    printf ("%s:  all done, var1 is %d, var2 is %d\n", progname, var1, var2);
    fflush (stdout);
    sleep (1);
    exit (0);
}

/*
 *  Текущий поток.
 *  The thread ensures that var1 == var2.  If this is not the
 *  case, the thread sets var1 = var2, and prints a message.
 *  Var1 and Var2 are incremented.
 *  Looking at the source, if there were no "synchronization" problems,
 *  then var1 would always be equal to var2.  Run this program and see
 *  what the actual result is...
*/

void do_work()
{
    static int var3;
    var3++;
    /* For faster/slower processors, may need to tune this program by
     * modifying the frequency of this printf -- add/remove a 0
     */
    if ( !(var3 % 10000000) ) 
        printf ("%s: thread %lu did some work\n", progname, (unsigned long)pthread_self());
}

void *update_thread (void *i)
{
    // На Linux зададим асинхронную отмену для корректного завершения
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
        pthread_mutex_lock(&mutex); // Залочил мютекс
        if (var1 != var2) {
            printf ("%s:  thread %ld, var1 (%d) is not equal to var2 (%d)!\n",
                    progname, (long) i, var1, var2);
            var1 = var2;
        }
        /* do some work here */
        do_work(); 
        var1++;
        //sched_yield(); /* for faster processors, to cause problem to happen */ 
        var2++;
        pthread_mutex_unlock(&mutex); // Заанлочил мютекс
    }
    return (NULL);
}
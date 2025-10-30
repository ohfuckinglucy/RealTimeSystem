/*
 *  This module demonstrates POSIX semaphores.
 *
 *  Operation:
 *      A counting semaphore is created, primed with 0 counts.
 *      Five consumer threads are started, each trying to obtain
 *      the semaphore.  
 *      A producer thread is created, which periodically posts
 *      the semaphore, unblocking one of the consumer threads.
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

sem_t   *mySemaphore;
void    *producer (void *);
void    *consumer (void *);
char    *progname = "semex";

#define SEM_NAME "/Semex"
// Для совместимости с Linux/macOS используем именованные семафоры по умолчанию
#define Named 1

int main ()
{
    int     i;
    setvbuf (stdout, NULL, _IOLBF, 0);
#ifdef  Named
    mySemaphore = sem_open (SEM_NAME, O_CREAT, S_IRWXU, 0);
    /* not sharing with other process, so immediately unlink */
    sem_unlink( SEM_NAME );
#else   // Named
    mySemaphore = malloc (sizeof (sem_t));
    sem_init (mySemaphore, 0, 0);
#endif  // Named

    pthread_t consumers[5];
    pthread_t producerThread;
    for (i = 0; i < 5; i++) {
        pthread_create (&consumers[i], NULL, consumer, (void *)(long)i);
    }
    pthread_create (&producerThread, NULL, producer, (void *) 1);
    sleep (20);     // let the threads run
    printf ("%s:  main, exiting\n", progname);
    return 0;
}

void *producer (void *i)
{
    while (1) {
        sleep (1);
        printf ("%s:  (producer %ld), posted semaphore\n", progname, (long)i);
        sem_post (mySemaphore);
    }
    return (NULL);
}

void *consumer (void *i)
{
    while (1) {
        sem_wait (mySemaphore);
        printf ("%s:  (consumer %ld) got semaphore\n", progname, (long)i);
    }
    return (NULL);
}
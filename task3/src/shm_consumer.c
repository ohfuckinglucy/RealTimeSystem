/*
 * Consumer (Потребитель) для Shared Memory IPC
 *
 * 1. Открывает существующий сегмент разделяемой памяти.
 * 2. Открывает существующие семафоры.
 * 3. В цикле читает данные из кольцевого буфера, когда они доступны.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include "shm_common.h"

volatile sig_atomic_t done = 0;
void term(int signum) {
    done = 1;
}

int main() {
    struct sigaction action;
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    // === 2. Открытие сегмента Shared Memory ===
    // Задержка, чтобы дать производителю время создать объекты
    sleep(1);
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    shared_data_t *shared_data = mmap(0, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    printf("Consumer: Shared memory segment opened and mapped.\n");

    sem_t *sem_prod = sem_open(SEM_PRODUCER, 0);
    if (sem_prod == SEM_FAILED) {
        perror("sem_open producer");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_cons = sem_open(SEM_CONSUMER, 0);
    if (sem_cons == SEM_FAILED) {
        perror("sem_open consumer");
        exit(EXIT_FAILURE);
    }
    printf("Consumer: Semaphores opened.\n");

    while (!done) {
        sem_wait(sem_cons);

        uint64_t value = shared_data->buffer[shared_data->tail];
        printf("Consumed: %llu from index %d\n", (unsigned long long)value, shared_data->tail);
        shared_data->tail = (shared_data->tail + 1) % BUFFER_SIZE;

        sem_post(sem_prod);
        
        usleep(200000); 
    }

    printf("\nConsumer: End of work...\n");

    munmap(shared_data, sizeof(shared_data_t));
    close(shm_fd);

    sem_close(sem_prod);
    sem_close(sem_cons);
    
    printf("Consumer: Resources freed.\n");
    return 0;
}

/*
    Без семафор prod и cons будут одновременно читать писать в общий буфер, что приведет
    к гонкам, поврежденным или пропущенным данным
    sem_prod ограничивает запись (BUFFER_SIZE)
    sem_)cons гарантирует, что читаем только после записи
*/
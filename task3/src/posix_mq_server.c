/*
 * Сервер POSIX Message Queues
 *
 * Ожидает сообщения от клиентов, преобразует их в верхний регистр
 * и отправляет обратно. Демонстрирует работу с приоритетами.
 */
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

void to_upper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

int main() {
    mqd_t mq_server, mq_client;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mq_unlink(SERVER_QUEUE_NAME);
    mq_unlink(CLIENT_QUEUE_NAME);

    mq_server = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mq_server == (mqd_t)-1) {
        perror("mq_open (server)");
        exit(1);
    }

    printf("Server is running and waiting for messages...\n");

    while (1) {
        char buffer[MAX_MSG_SIZE];
        unsigned int priority;

        ssize_t bytes_read = mq_receive(mq_server, buffer, MAX_MSG_SIZE, &priority);
        if (bytes_read >= 0) {
            buffer[bytes_read] = '\0';
            printf("Received message with priority %u: \"%s\"\n", priority, buffer);

            to_upper(buffer);

            mq_client = mq_open(CLIENT_QUEUE_NAME, O_WRONLY);
            if (mq_client == (mqd_t)-1) {
                perror("mq_open (client)");
                continue; 
            }

            if (mq_send(mq_client, buffer, strlen(buffer) + 1, 0) == -1) {
                perror("mq_send");
            } else {
                printf("Sent answer: \"%s\"\n", buffer);
            }
            mq_close(mq_client);

        } else {
            perror("mq_receive");
        }
    }

    mq_close(mq_server);
    mq_unlink(SERVER_QUEUE_NAME);

    return 0;
}

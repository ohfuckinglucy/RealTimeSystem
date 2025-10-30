/*
 * Клиент POSIX Message Queues
 *
 * Отправляет сообщения на сервер и ждет ответа.
 * Демонстрирует отправку сообщений с разными приоритетами.
 */
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

int main() {
    mqd_t mq_server, mq_client;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mq_server = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (mq_server == (mqd_t)-1) {
        perror("mq_open (server)");
        exit(1);
    }

    mq_client = mq_open(CLIENT_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq_client == (mqd_t)-1) {
        perror("mq_open (client)");
        exit(1);
    }

    char *messages[] = {"ordinary message 1", "urgent message!", "ordinary message 2"};
    unsigned int priorities[] = {MSG_PRIO_NORMAL, MSG_PRIO_HIGH, MSG_PRIO_NORMAL};

    for (int i = 0; i < 3; ++i) {
        printf("Send message with priority %u: \"%s\"\n", priorities[i], messages[i]);
        if (mq_send(mq_server, messages[i], strlen(messages[i]) + 1, priorities[i]) == -1) {
            perror("mq_send");
            continue;
        }

        char buffer[MAX_MSG_SIZE];
        if (mq_receive(mq_client, buffer, MAX_MSG_SIZE, NULL) >= 0) {
            printf("Received answer: \"%s\"\n\n", buffer);
        } else {
            perror("mq_receive");
        }
        sleep(1); 
    }

    mq_close(mq_server);
    mq_close(mq_client);
    mq_unlink(CLIENT_QUEUE_NAME);

    return 0;
}

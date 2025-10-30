/*
 * Демонстрация векторного (scatter-gather) I/O с readv/writev
 *
 * Цель: Показать, как можно записать или прочитать несколько
 * отдельных буферов в памяти за один системный вызов, избегая
 * необходимости предварительно копировать их в единый смежный буфер.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <stdint.h>
#include <arpa/inet.h> // для ntohl/htonl

typedef struct {
    uint32_t msg_type;
    uint64_t msg_id;
    char payload[20];
} message_t;

int main() {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    printf("--- WRITER ---\n");
    message_t msg_to_send = {
        .msg_type = htonl(1), // Тип "DATA"
        .msg_id = 9988776655443322L,
    };
    strncpy(msg_to_send.payload, "Hello, IOV!", sizeof(msg_to_send.payload));
    
    printf("Preparing to send:\n");
    printf("  msg_type: %u\n", ntohl(msg_to_send.msg_type));
    printf("  msg_id:   %llu\n", (unsigned long long)msg_to_send.msg_id);
    printf("  payload:  \"%s\"\n", msg_to_send.payload);

    struct iovec iov_write[3];
    iov_write[0].iov_base = &msg_to_send.msg_type;
    iov_write[0].iov_len = sizeof(msg_to_send.msg_type);
    
    iov_write[1].iov_base = &msg_to_send.msg_id;
    iov_write[1].iov_len = sizeof(msg_to_send.msg_id);
    
    iov_write[2].iov_base = msg_to_send.payload;
    iov_write[2].iov_len = strlen(msg_to_send.payload);

    ssize_t bytes_written = writev(pipe_fd[1], iov_write, 3);
    if (bytes_written == -1) {
        perror("writev");
        exit(EXIT_FAILURE);
    }
    printf("writev() wrote %zd bytes.\n\n", bytes_written);
    close(pipe_fd[1]); 


    printf("--- READER ---\n");
    message_t msg_received = {0}; 

    struct iovec iov_read[3];
    iov_read[0].iov_base = &msg_received.msg_type;
    iov_read[0].iov_len = sizeof(msg_received.msg_type);

    iov_read[1].iov_base = &msg_received.msg_id;
    iov_read[1].iov_len = sizeof(msg_received.msg_id);

    iov_read[2].iov_base = msg_received.payload;
    iov_read[2].iov_len = strlen(msg_to_send.payload);

    ssize_t bytes_read = readv(pipe_fd[0], iov_read, 3);
    if (bytes_read == -1) {
        perror("readv");
        exit(EXIT_FAILURE);
    }
    printf("readv() read %zd bytes.\n", bytes_read);

    printf("Received data:\n");
    printf("  msg_type: %u\n", ntohl(msg_received.msg_type));
    printf("  msg_id:   %llu\n", (unsigned long long)msg_received.msg_id);
    printf("  payload:  \"%s\"\n", msg_received.payload);
    
    close(pipe_fd[0]);

    if (ntohl(msg_received.msg_type) == 1 &&
        msg_received.msg_id == 9988776655443322L &&
        strcmp(msg_received.payload, "Hello, IOV!") == 0) {
        printf("\nCheck passed: data matches.\n");
    } else {
        printf("\nCheck failed: data does not match.\n");
    }

    return 0;
}

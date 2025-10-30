/*
 * Асинхронный сервер на epoll, UNIX-сокетах и eventfd
 *
 * Цель: Показать, как эффективно управлять множеством файловых дескрипторов:
 *  - Слушающий сокет для новых клиентов.
 *  - Сокеты подключенных клиентов для чтения данных.
 *  - eventfd для внутренних уведомлений (например, от других потоков).
 *  - Корректная обработка отключения клиента.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/eventfd.h>
#include <errno.h>

#define MAX_EVENTS 10
#define SOCKET_PATH "/tmp/epoll_server.sock"
#define READ_BUFFER_SIZE 256

void add_to_epoll(int epoll_fd, int fd, uint32_t events) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("epoll_ctl ADD");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int server_fd, epoll_fd, event_fd;
    struct sockaddr_un addr;
    struct epoll_event events[MAX_EVENTS];

    unlink(SOCKET_PATH); 
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on socket: %s\n", SOCKET_PATH);

    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    if ((event_fd = eventfd(0, EFD_NONBLOCK)) == -1) {
        perror("eventfd");
        exit(EXIT_FAILURE);
    }
    printf("Created eventfd, to emulate internal event execute:\n");
    printf("echo 1 > /proc/%d/fd/%d\n\n", getpid(), event_fd);


    add_to_epoll(epoll_fd, server_fd, EPOLLIN);
    add_to_epoll(epoll_fd, event_fd, EPOLLIN);

    while (1) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_events == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < n_events; i++) {
            if (events[i].data.fd == server_fd) {
                // --- Новое подключение ---
                int client_fd = accept4(server_fd, NULL, NULL, SOCK_NONBLOCK);
                if (client_fd == -1) {
                    perror("accept4");
                    continue;
                }
                add_to_epoll(epoll_fd, client_fd, EPOLLIN | EPOLLET); // ET для примера
                printf("New client (fd=%d) connected.\n", client_fd);

            } else if (events[i].data.fd == event_fd) {
                // --- Внутреннее событие ---
                uint64_t counter;
                read(event_fd, &counter, sizeof(counter)); // Сбрасываем счетчик
                printf("!!! Received internal event (counter=%llu) !!!\n", (unsigned long long)counter);

            } else {
                int client_fd = events[i].data.fd;
                char buffer[READ_BUFFER_SIZE];

                ssize_t bytes_read = read(client_fd, buffer, READ_BUFFER_SIZE);

                if (bytes_read == -1) {
                    // EWOULDBLOCK означает, что мы прочитали все данные (в режиме ET)
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        perror("read");
                        close(client_fd);
                    }
                } else if (bytes_read == 0) {
                    // --- Обрыв соединения ---
                    // Клиент закрыл сокет. epoll автоматически удаляет fd,
                    // но мы должны его закрыть сами.
                    printf("Client (fd=%d) disconnected.\n", client_fd);
                    close(client_fd); // epoll_ctl(EPOLL_CTL_DEL) не нужен для close
                } else {
                    buffer[bytes_read] = '\0';
                    printf("Received from client (fd=%d): %s", client_fd, buffer);
                    // Эхо-ответ
                    write(client_fd, buffer, bytes_read);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    close(event_fd);
    unlink(SOCKET_PATH);

    return 0;
}

/*
 * Как протестировать:
 * 1. Запустите сервер: ./bin/epoll_server
 * 2. В другом терминале подключитесь клиентом: socat - UNIX-CONNECT:/tmp/epoll_server.sock
 *    - Набирайте текст и нажимайте Enter. Сервер должен вернуть его обратно.
 *    - Закройте socat (Ctrl+C). Сервер должен сообщить об отключении.
 * 3. В третьем терминале, чтобы проверить eventfd, выполните команду,
 *    которую сервер вывел при старте (echo 1 > /proc/...).
 *    Сервер должен сообщить о внутреннем событии.
 */

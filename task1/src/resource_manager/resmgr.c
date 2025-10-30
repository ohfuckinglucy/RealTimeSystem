#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>

#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock"
#define DEVICE_SIZE 1024

#define FLAG_READ  1
#define FLAG_WRITE 2

static const char *progname = "example";
static int optv = 0;
static int listen_fd = -1;

static char device_buffer[DEVICE_SIZE];
static size_t device_cursor = 0;

struct client_ocb {
    size_t position;
    int flags;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void options(int argc, char *argv[]);
static void install_signals(void);
static void on_signal(int signo);
static void *client_thread(void *arg);

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("%s: starting...\n", progname);

    options(argc, argv);
    install_signals();

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) { perror("socket"); return EXIT_FAILURE; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);

    unlink(EXAMPLE_SOCK_PATH);
    umask(0);
    
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind"); close(listen_fd); return EXIT_FAILURE;
    }
    chmod(EXAMPLE_SOCK_PATH, 0666);

    if (listen(listen_fd, 8) == -1) {
        perror("listen"); close(listen_fd); unlink(EXAMPLE_SOCK_PATH); return EXIT_FAILURE;
    }

    printf("%s: listening on %s\n", progname, EXAMPLE_SOCK_PATH);
    printf("Подключитесь клиентом (например: `nc -U %s`) и отправьте данные.\n", EXAMPLE_SOCK_PATH);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd == -1) { if (errno == EINTR) continue; perror("accept"); break; }
        if (optv) printf("%s: io_open — новое подключение (fd=%d)\n", progname, client_fd);

        pthread_t th;
        if (pthread_create(&th, NULL, client_thread, (void *)(long)client_fd) != 0) {
            perror("pthread_create"); close(client_fd); continue;
        }
        pthread_detach(th);
    }

    if (listen_fd != -1) close(listen_fd);
    unlink(EXAMPLE_SOCK_PATH);
    return EXIT_SUCCESS;
}

static void *client_thread(void *arg) {
    int fd = (int)(long)arg;
    char buf[1024];

    struct client_ocb ocb;
    ocb.position = 0;
    ocb.flags = FLAG_READ | FLAG_WRITE;

    for (;;) {
        ssize_t n = recv(fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) break;

        if (optv) printf("%s: io_read — %zd байт: '%s'\n", progname, n, buf);

        if (strncmp(buf, "WRITE", 5) == 0) {
            if (!(ocb.flags & FLAG_WRITE)) {
                send(fd, "ERR: no write permission\n", 25, 0);
                continue;
            }

            char *data = buf + 6;
            size_t len = strlen(data);

            pthread_mutex_lock(&mutex);
            if (device_cursor + len >= DEVICE_SIZE) {
                pthread_mutex_unlock(&mutex);
                send(fd, "ERR: overflow\n", 14, 0);
                continue;
            }
            memcpy(device_buffer + device_cursor, data, len);
            device_cursor += len;
            device_buffer[device_cursor] = '\0';
            pthread_mutex_unlock(&mutex);

            send(fd, "OK\n", 3, 0);
        }
        else if (strncmp(buf, "READ", 4) == 0) {
            if (!(ocb.flags & FLAG_READ)) {
                send(fd, "ERR: no read permission\n", 24, 0);
                continue;
            }

            pthread_mutex_lock(&mutex);
            size_t len = device_cursor - ocb.position;
            if (len > 0) {
                send(fd, device_buffer + ocb.position, len, 0);
                ocb.position += len;
            }
            pthread_mutex_unlock(&mutex);

            send(fd, "\nOK\n", 4, 0);
        }
        else if (strncmp(buf, "CLEAR", 5) == 0) {
            pthread_mutex_lock(&mutex);
            device_cursor = 0;
            memset(device_buffer, 0, DEVICE_SIZE);
            pthread_mutex_unlock(&mutex);
            ocb.position = 0;
            send(fd, "OK\n", 3, 0);
        }
        else if (strncmp(buf, "STATUS", 6) == 0) {
            char status[128];
            pthread_mutex_lock(&mutex);
            int len = snprintf(status, sizeof(status), "Cursor: %zu, Buf_len: %zu, Flags: %s%s\n", device_cursor, strlen(device_buffer), (ocb.flags & FLAG_READ) ? "R" : "", (ocb.flags & FLAG_WRITE) ? "W" : "");
            pthread_mutex_unlock(&mutex);
            send(fd, status, len, 0);
        }
        else if (strncmp(buf, "SETFLAG", 7) == 0) {
            char *arg = buf + 8;
            if (strcmp(arg, "READ") == 0) ocb.flags |= FLAG_READ;
            else if (strcmp(arg, "WRITE") == 0) ocb.flags |= FLAG_WRITE;
            else if (strcmp(arg, "NOREAD") == 0) ocb.flags &= ~FLAG_READ;
            else if (strcmp(arg, "NOWRITE") == 0) ocb.flags &= ~FLAG_WRITE;
            else { send(fd, "ERR: unknown flag\n", 18, 0); continue; }
            send(fd, "OK\n", 3, 0);
        }
        else if (strncmp(buf, "EXIT", 4) == 0) {
            send(fd, "BYE\n", 4, 0);
            break;
        }
        else if (strncmp(buf, "SEEK", 4) == 0) {
            char *arg = buf + 5;
            long pos = strtol(arg, NULL, 10);
            pthread_mutex_lock(&mutex);
            if (pos < 0 || (size_t)pos > device_cursor) {
                pthread_mutex_unlock(&mutex);
                send(fd, "ERR: invalid position\n", 22, 0);
            } else {
                ocb.position = (size_t)pos;
                pthread_mutex_unlock(&mutex);
                send(fd, "OK\n", 3, 0);
            }
        }
        else {
            send(fd, "ERR: unknown command\n", 21, 0);
        }
    }

    if (optv) printf("%s: клиент отключился (fd=%d)\n", progname, fd);
    close(fd);
    return NULL;
}

static void options(int argc, char *argv[]) {
    int opt; optv = 0;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) { case 'v': optv++; break; }
    }
}

static void install_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void on_signal(int signo) {
    (void)signo;
    if (listen_fd != -1) close(listen_fd);
    unlink(EXAMPLE_SOCK_PATH);
    fprintf(stderr, "\n%s: завершение по сигналу\n", progname);
    _exit(0);
}

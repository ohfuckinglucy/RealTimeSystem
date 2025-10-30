#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>
#include <sys/ioctl.h>

#define MAX_DEVICES 16

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX1 /dev/input/eventX2 ...\n", argv[0]);
        return 1;
    }

    if (argc - 1 > MAX_DEVICES) {
        fprintf(stderr, "Error: Too many devices. Max is %d.\n", MAX_DEVICES);
        return 1;
    }

    int num_devices = argc - 1;
    struct pollfd fds[MAX_DEVICES];
    char device_names[MAX_DEVICES][256];

    // Открыть все переданные устройства
    for (int i = 0; i < num_devices; i++) {
        const char *path = argv[i + 1];
        int fd = open(path, O_RDONLY | O_NONBLOCK); // O_NONBLOCK важен для poll
        if (fd < 0) {
            perror("Failed to open device");
            // В реальном приложении здесь нужна более внимательная обработка ошибок
            return 1;
        }

        fds[i].fd = fd;
        fds[i].events = POLLIN; // Ждем только события чтения

        // Получаем имя устройства для красивого вывода
        ioctl(fd, EVIOCGNAME(sizeof(device_names[i])), device_names[i]);
    }

    printf("Polling %d devices. Press Ctrl+C to exit.\n", num_devices);

    while (1) {
        // Вызвать poll() для ожидания событий
        int ret = poll(fds, num_devices, -1); // -1 означает бесконечное ожидание
        if (ret < 0) {
            perror("poll failed");
            break;
        }

        // Проверить, на каких файловых дескрипторах произошли события
        for (int i = 0; i < num_devices; i++) {
            if (fds[i].revents & POLLIN) {
                struct input_event ev;
                ssize_t bytes = read(fds[i].fd, &ev, sizeof(ev));
                if (bytes == sizeof(ev)) {
                    printf("Event from %s: type %d, code %d, value %d\n",
                           device_names[i], ev.type, ev.code, ev.value);
                }
            }
        }
    }

    // Закрыть все файловые дескрипторы
    for (int i = 0; i < num_devices; i++) {
        close(fds[i].fd);
    }

    return 0;
}

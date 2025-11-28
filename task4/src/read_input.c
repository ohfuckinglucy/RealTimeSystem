#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

// Для Задания 2:
#include <string.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX\n", argv[0]);
        return 1;
    }

    const char *device_path = argv[1];

    // Открыть файл устройства для чтения (O_RDONLY)
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /* --- ЗАДАНИЕ 2: ИСПОЛЬЗОВАНИЕ IOCTL --- */
    // Создать буфер для имени устройства
    // Использовать ioctl с EVIOCGNAME для получения имени
    // Вывести имя устройства

    char name[256];

    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0){
        perror("ioctl EVIOCGNAME");
    } else {
        printf("Device name: %s\n", name);
    }

    printf("Reading events from %s. Press Ctrl+C to exit.\n", device_path);

    struct input_event ev;
    while (1) {
        // Прочитать структуру input_event из файла устройства
        ssize_t bytes = read(fd, &ev, sizeof(struct input_event));
        if (bytes != sizeof(struct input_event)) {
            perror("Failed to read event");
            break;
        }

        // Выводим информацию о событии
        // Для более осмысленного вывода можно смотреть в linux/input-event-codes.h
        if (ev.type == EV_KEY) { // Интересуют только события клавиатуры
             printf("Event: type %d, code %d, value %d\n", ev.type, ev.code, ev.value);
        }
    }

    close(fd);
    return 0;
}

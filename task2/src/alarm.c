/*
 * Демонстрация базовой функции alarm().
 *
 * alarm() отправляет сигнал SIGALRM процессу
 * по истечении заданного времени, что по умолчанию завершает процесс.
 *
 * ВАЖНО: alarm() считается устаревшим и ненадежным для серьезных
 * задач реального времени из-за его низкой точности и использования
 * сигналов, которые могут быть потеряны.
 */
#include <stdio.h>
#include <unistd.h>

char *progname = "alarm";

int main() {
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("%s: Setting timer (alarm) for 2 seconds.\n", progname);
    // alarm() асинхронно попросит ядро послать нам сигнал SIGALRM через 2 сек.
    alarm(3);

    printf("%s: Timer set. Now process will sleep for 5 seconds.\n", progname);
    // sleep() может быть прерван сигналом, но даже если этого не произойдет,
    // SIGALRM "догонит" процесс сразу после пробуждения или во время работы.
    sleep(1);

    // Эта строка никогда не должна выполниться, так как стандартное действие
    // для SIGALRM - завершение (Terminate) процесса.
    printf("%s: This line should not be executed!\n", progname);
    return 0;
}
/*
 * Демонстрация работы с системными часами POSIX.
 *
 * Цель: Показать базовые функции для работы со временем,
 * такие как clock_gettime().
 *
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> // Для clock_gettime

#define BILLION 1000000000LL
#define NumSamples 10

char *progname = "calctime1";

int main() {
    struct timespec clockval, prevclockval;
    uint64_t delta;

    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("\nMeasuring the minimum possible delta for CLOCK_REALTIME:\n");

    // В этом цикле мы пытаемся "поймать" момент смены показаний системных часов,
    // чтобы эмпирически оценить их разрешение.
    for (int i = 0; i < NumSamples; i++) {
        clock_gettime(CLOCK_REALTIME, &prevclockval);
        do {
            clock_gettime(CLOCK_REALTIME, &clockval);
        } while (clockval.tv_sec == prevclockval.tv_sec &&
                 clockval.tv_nsec == prevclockval.tv_nsec);

        delta = ((uint64_t)clockval.tv_sec * BILLION + (uint64_t)clockval.tv_nsec) -
                ((uint64_t)prevclockval.tv_sec * BILLION +
                 (uint64_t)prevclockval.tv_nsec);

        printf("prev %ld.%09ld, new %ld.%09ld, delta %" PRIu64 " ns\n",
               prevclockval.tv_sec, prevclockval.tv_nsec, clockval.tv_sec,
               clockval.tv_nsec, delta);
    }

    printf("\n%s: End of work.\n", progname);
    return EXIT_SUCCESS;
}
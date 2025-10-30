#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
int main(int argc, char* argv[])
{
  int i = 0, j;
  setvbuf(stdout, NULL, _IOLBF, 0);
  if (argc == 1) {
    printf("Программу следует запускать с аргументами командной строки, например:\n\n");
    printf("hello Это пример текста\n");
    return EXIT_FAILURE;
  }
  while (1) {
    printf("#%d: ", i++);
    for (j = 1; j < argc; j++) {
      printf("%s ", argv[j]);
    }
    printf("\n");
    sleep(1);
  }
  return EXIT_SUCCESS;
}
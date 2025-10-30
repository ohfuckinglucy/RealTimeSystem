#include <stdlib.h> // константы, EXIT_SUCCESS
#include <stdio.h>  // стандартный ввод/вывод (printf, perror)
#include <pthread.h> // потоки POSIX
#include <sched.h>   // приоритеты потоков
#include <unistd.h>  // pause(), usleep()
 
void *sense(void* arg); 
void *stateOutput(void* arg); 
void *userInterface(void* arg); 
short isRealState(char s); 
 
char state; 
short changed; 
pthread_cond_t stateCond; 
pthread_mutex_t stateMutex; 
 
#define TRUE 1 
#define FALSE 0 


 
int main(int argc, char *argv[]) { 
 
    printf("Hello World!\n"); 
    
    // Инициализация переменных
    state = 'N'; 
    pthread_cond_init(&stateCond, NULL); 
    pthread_mutex_init(&stateMutex, NULL); 
    
    // Идентификаторы потоков
    pthread_t sensorThread; 
    pthread_t stateOutputThread; 
    pthread_t userThread; 
    
    // Запуск потоков
    pthread_create(&sensorThread, NULL, sense, NULL); 
    pthread_create(&stateOutputThread, NULL, stateOutput, NULL); 
    pthread_create(&userThread, NULL, userInterface, NULL); 
    
    // Ожидание CTRL + C (остальные потоки будут работать до завершения
    // или до получения сигнала)
    pause(); 
    
    // Сюда поток не дойдёт из-за бесконечных циклов. Для корректного
    // завершения используйте булевы флаги вместо констант TRUE.
    
    printf("Exit Success!\n"); 
    
    return EXIT_SUCCESS; 
} 
 
// Поток "sense" определяет, изменилось ли состояние.
// С точки зрения условных переменных — это сигнальный поток.
void *sense(void* arg) { 
char prevState = ' '; 
while (TRUE) { 
// Считываем символ, но не меняем глобальную переменную state,
// пока не убедимся, что это допустимое состояние
char tempState; 
scanf("%c", &tempState); 
usleep(10 * 1000);
 
// Блокируем мьютекс перед модификацией
pthread_mutex_lock(&stateMutex); 
 
if (isRealState(tempState)) {
state = tempState;
}
 
// Если состояние изменилось — уведомляем поток stateOutput
// (примечание: state ^ ' ' инвертирует регистр символа: x -> X)
if (prevState != state && prevState != (state ^ ' ')) { 
// Устанавливаем флаг для ожидающего потока
changed = TRUE; 
// Сигнализируем ожидающему потоку (stateOutput)
pthread_cond_signal(&stateCond); 
} 
 
// Сохраняем текущее состояние
prevState = state; 
 
// Разблокируем мьютекс
pthread_mutex_unlock(&stateMutex); 
} 
return NULL; 
} 
 
// Вспомогательная функция проверки допустимого состояния
short isRealState(char s) { 
short real = FALSE; 
 
if (s == 'R' || s == 'r') //Ready 
real = TRUE; 
else if (s == 'N' || s == 'n') //Not Ready 
real = TRUE; 
else if (s == 'D' || s == 'd') //Run Mode 
real = TRUE; 
 
return real; 
} 
 
// Поток "stateOutput" выводит изменение состояния.
// С точки зрения условных переменных — это ожидающий поток.
void *stateOutput(void* arg) { 
changed = FALSE; 
while (TRUE) { 
// Блокировка мьютекса перед ожиданием
pthread_mutex_lock(&stateMutex); 
 
// Если состояние не изменилось, ждём (используем while для устойчивости)
while (!changed) { 
// Ждём сигнала
pthread_cond_wait(&stateCond, &stateMutex); 
} 
 
// Вывод нового состояния
printf("The state has changed! It is now in "); 
if (state == 'n' || state == 'N') //Not ready 
printf("Not Ready State\n"); 
else if (state == 'r' || state == 'R') //Ready 
printf("Ready State\n"); 
else if (state == 'd' || state == 'D') //Run Mode 
printf("Run Mode\n"); 
 
// Сбрасываем флаг 'changed' и снова ждём изменений
changed = FALSE; 
 
// Разблокировка мьютекса
pthread_mutex_unlock(&stateMutex); 
} 
return NULL; 
} 
 
// Поток "userInterface" выводит простую визуализацию состояния
void *userInterface(void* arg) { 
while (TRUE) { 
if (state == 'n' || state == 'N') //Not ready 
printf("___________________________________________________\n"); 
else if (state == 'r' || state == 'R') //Ready 
printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); 
else if (state == 'd' || state == 'D') //Run Mode 
printf("\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/\\n"); 
usleep(1000 * 1000);
} 
return NULL; 
}
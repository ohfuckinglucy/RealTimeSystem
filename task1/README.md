# Практическое задание 1 — Организация доступа к общим ресурсам

В этом задании вы познакомитесь с базовыми механизмами организации доступа к общим ресурсам в многопоточном окружении, семафорами, условными переменными, а также разберёте проблему инверсии приоритетов и обработку прерываний/сигналов.

## Структура

- `src/intro`: Hello World и базовая работа с потоками/условными переменными
- `src/shared_mem`: доступ к разделяемому ресурсу (без синхронизации, мьютекс, семафоры, условные переменные, producer-consumer)
- `src/inv_prio`: демонстрация инверсии приоритетов и фикс (наследование приоритета)
- `src/interrupt`: обработка сигналов и периодический таймер как аналог прерываний
- `src/resource_manager`: скелет менеджера ресурсов на UNIX-сокетах и простой клиент

## Сборка

Компилировать можно по отдельности. Примеры (Linux/macOS, требуется `gcc`, `pthread`):

```bash
# intro
cc -Wall -Wextra -O2 -pthread src/intro/hello.c -o bin/hello
cc -Wall -Wextra -O2 -pthread src/intro/intro.c  -o bin/intro

# shared_mem
cc -Wall -Wextra -O2 -pthread src/shared_mem/nomutex.c  -o bin/nomutex
cc -Wall -Wextra -O2 -pthread src/shared_mem/semex.c    -o bin/semex
cc -Wall -Wextra -O2 -pthread src/shared_mem/condvar.c  -o bin/condvar
cc -Wall -Wextra -O2 -pthread src/shared_mem/prodcons.c -o bin/prodcons

# interrupt
cc -Wall -Wextra -O2 -pthread src/interrupt/intsimple.c -o bin/intsimple
cc -Wall -Wextra -O2 -pthread src/interrupt/int.c       -o bin/int

# inv_prio (лучше запускать на Linux с root для SCHED_FIFO)
cc -Wall -Wextra -O2 -pthread src/inv_prio/working.c src/inv_prio/scenario_1.c -o bin/inv_s1
cc -Wall -Wextra -O2 -pthread src/inv_prio/working.c src/inv_prio/scenario_2.c -o bin/inv_s2

# resource_manager
cc -Wall -Wextra -O2 -pthread src/resource_manager/resmgr.c -o bin/resmgr
cc -Wall -Wextra -O2 -pthread src/resource_manager/client.c -o bin/resmgr_client
```
Создайте каталог `bin/` перед сборкой:

```bash
mkdir -p bin
```

## Запуск (примеры)

- intro/hello: `./bin/hello Привет мир`
- intro/intro: `./bin/intro` (вводите символы состояний: R/N/D)
- shared_mem/nomutex: `./bin/nomutex`
- shared_mem/semex: `./bin/semex`
- shared_mem/condvar: `./bin/condvar`
- shared_mem/prodcons: `./bin/prodcons`
- interrupt/intsimple: `./bin/intsimple` (нажимайте клавиши, шлите SIGUSR1/2, SIGTERM)
- interrupt/int: `./bin/int` (печать каждых 100 тиков SIGALRM)
- inv_prio/scenario_1: `sudo ./bin/inv_s1` (наблюдайте задержку у высокого приоритета)
- inv_prio/scenario_2: подготовлено для самостоятельной работы
- resource_manager: в одном терминале `./bin/resmgr`, в другом — `./bin/resmgr_client "hello"`

## Теоретические сведения

- Мьютексы: взаимное исключение, критические секции, протоколы PTHREAD_PRIO_INHERIT/PTHREAD_PRIO_PROTECT (при наличии поддержки).
- Семафоры: счётные/двоичные, операции `sem_wait/sem_post`, справедливая разблокировка не гарантируется.
- Условные переменные: ожидание событий под мьютексом, схема wait-loop, spurious wakeups.
- Producer-Consumer: координация двух потоков, смена состояний и буферизация.
- Инверсия приоритетов: высокий поток ждёт ресурс у низкого, а средний вытесняет — решается наследованием приоритета.
- Сигналы/таймеры: SIGUSR1/2, SIGTERM, SIGALRM через `setitimer`, обработчики — максимально простые, без небезопасных вызовов.
- Менеджер ресурсов: на Linux имитирован через UNIX-сокеты; open/read/write ≈ accept/recv/send.

Подробности см. в README в подпапках.

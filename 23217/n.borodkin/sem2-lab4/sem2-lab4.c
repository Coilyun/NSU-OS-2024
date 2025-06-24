#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

void *print_str(void *arg) {
    while (1) {
        puts("Дочерняя нить работает");
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int res = pthread_create(&thread, NULL, print_str, NULL);
    if (res != 0) {
        fputs("Ошибка создания потока\n", stderr);
        return EXIT_FAILURE;
    }

    sleep(2);

    res = pthread_cancel(thread);
    if (res != 0) {
        fputs("Ошибка завершения потока\n", stderr);
        return EXIT_FAILURE;
    }

    res = pthread_join(thread, NULL);
    if (res != 0) {
        fputs("Ошибка завершения ожидания потока\n", stderr);
        return EXIT_FAILURE;
    }
    
    puts("Программа завершена");

    return EXIT_SUCCESS;
}

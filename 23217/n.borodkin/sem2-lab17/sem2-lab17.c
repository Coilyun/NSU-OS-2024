#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <pthread.h>
#include <unistd.h>


//Структура связного списка
struct node{
    char *head;
    struct node *next;
};

//Структура потока
struct potok{
    struct node **node;
    pthread_mutex_t *mutex;
};

volatile int stop_flag = 0; // volatile заставляет компилятор не оптимизировать запись/чтение в переменную

//Добавление в начало списка
int add(struct node **old_node, char *text, int len){
    struct node *new_node = (struct node*)malloc(sizeof(struct node));
    
    if(new_node == NULL){
        perror("Ошибка выделения памяти для нового узла");
        return -1;
    }

    new_node->head = (char *)malloc(len);

    if(new_node->head == NULL){
        perror("Ошибка выделения памяти для строки");
        free(new_node);
        return -1;
    }

    strncpy(new_node->head, text, len);

    new_node->next = *old_node;
    *old_node = new_node;

    return 0;
}

//Режем строки по 80 и кладем в св. список(если строка <= 80 просто кладем)
int cut_into_80(struct node **node, char *buffer){
    int len = strlen(buffer);
    if(len <= 80){
        add(node, buffer, len);
    }
    else{
        int i = 0;

        while(i < len){
            int temp_size;

            if(len - i >= 80){
                temp_size = 80;
            }
            else{
                temp_size = len - i;
            }

            char saved_char = buffer[i + temp_size];
            buffer[i + temp_size] = '\0';
            
            if(add(node, &buffer[i], temp_size) != 0){
                buffer[i + temp_size] = saved_char;
                return -1;
            }
            buffer[i + temp_size] = saved_char;
            i += temp_size;
        }
    }
    return 0;
}

//Печатаем буфер
void print_buffer(struct node *node){
    struct node *current_node = node;
    printf("Содержимое буфера\n");
    printf("----------------\n");
    while(current_node){
        printf("%s\n", current_node->head);
        current_node = current_node->next;
    }
    printf("----------------\n");
}

//Освобождаем связной список
void free_node(struct node *node){
    while(node){
        struct node *copy_of_node = node; // тут сохраняем указатель на ноду
        node = node->next;
        free(copy_of_node->head);
        free(copy_of_node);
    }
}

//Пузырьковая сортировка для св. списка
void bubble_sort(struct node *node){
    if(node == NULL){
        return;
    }

    int swapped = 1;

    struct node *current_node;
    struct node *max_node = NULL;
    while(swapped){
        swapped = 0;
        current_node = node;

        while(current_node->next != max_node){ // почему max_node а не NULL, чтобы шли не до конца списка, а до последнего стоящего на своём месте(это мини оптимизация)
            if(strcmp(current_node->head, current_node->next->head) > 0){
                char *temp = current_node->head;
                current_node->head = current_node->next->head;
                current_node->next->head = temp;

                swapped = 1;
            }

            current_node = current_node->next;
        }
        max_node = current_node;

        if(swapped == 0){
            break;
        }
    }
}

//Функция дочернего потока
void *go_sort(void *args){
    struct potok *potok = (struct potok *)args;
    while(!stop_flag){
        sleep(5);

        pthread_mutex_lock(potok->mutex);

        bubble_sort(*potok->node);

        pthread_mutex_unlock(potok->mutex);
    }
    return 0;
}

int main(){
    pthread_mutex_t mutex;
    int res = pthread_mutex_init(&mutex, NULL);
    if(res != 0){
        fprintf(stderr, "Ошибка инициализации мьютекса: %s\n", strerror(res));
        return EXIT_FAILURE;
    }

    struct node *new_node = NULL;

    struct potok args;
    args.mutex = &mutex;
    args.node = &new_node;

    pthread_t thread;
    res = pthread_create(&thread, NULL, go_sort, &args);
    if(res != 0){
        fprintf(stderr, "Ошибка создания потока: %s\n", strerror(res));
        pthread_mutex_destroy(&mutex);
        return EXIT_FAILURE;
    }

    char buffer[1000];

    int resources_freed = 0; // флаг проверки освобожденности ресурсов
    while(1){
        if((fgets(buffer, sizeof(buffer), stdin)) == NULL){
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0; // тут если был символ переноса строки, то заменили на нулевой, а если не было то заменили нулевой на нулевой:)

        if(strlen(buffer) == 0){
            res = pthread_mutex_lock(&mutex);
            if(res != 0){
                fprintf(stderr, "Ошибка блокировки мьютекса: %s\n", strerror(res));
                break;
            }
            print_buffer(new_node);
            pthread_mutex_unlock(&mutex);
            continue; //используем чтобы начать цикл с начала и не попасть на второй unlock мьютекса и не делать внос пустой строки
        }

        if(cut_into_80(&new_node, buffer) != 0){
            pthread_mutex_unlock(&mutex);
            fprintf(stderr, "Ошибка при добавлении строки\n");
            exit(EXIT_FAILURE);
        }  

        int res4 = pthread_mutex_unlock(&mutex);
        if(res4 != 0){
            fprintf(stderr, "Ошибка разблокировки мьютекса: %s\n", strerror(res4));
            break;
        }

        if(!strcmp(buffer, "стоп")){ // просто слово чтобы завершить программу
            stop_flag = 1;
            pthread_join(thread, NULL);

            free_node(new_node);

            pthread_mutex_destroy(&mutex);
            resources_freed = 1;
            break;
        }
    }
    if(!resources_freed){
        stop_flag = 1;
        pthread_join(thread, NULL);
        free_node(new_node);
        pthread_mutex_destroy(&mutex);
    }
    
    return 0;
}

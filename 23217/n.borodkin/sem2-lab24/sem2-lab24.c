#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

sem_t sem_A, sem_B, sem_C, sem_Module;
volatile sig_atomic_t stop = 0;

void handle_signal(int sig){
    stop = 1;
}

void *detail_A(void *arg){
    while(!stop){
        sleep(1);
        if(sem_post(&sem_A) != 0){
            fputs("Sem_post_err - Sem_A\n", stderr);
            return (void *)1;
        }
        puts("Detail 'A' completed");
    }
    return NULL;
}
void *detail_B(void *arg){
    while(!stop){
        sleep(2);
        if(sem_post(&sem_B) != 0){
            fputs("Sem_post_err - Sem_B\n", stderr);
            return (void *)1;
        }
        puts("Detail 'B' completed");
    }
    return NULL;
}
void *detail_C(void *arg){
    while(!stop){
        sleep(3);
        if(sem_post(&sem_C) != 0){
            fputs("Sem_post_err - Sem_C\n", stderr);
            return (void *)1;
        }
        puts("Detail 'C' completed");
    }
    return NULL;
}
void *module(void *arg){
    while(!stop){
        if(sem_wait(&sem_A) != 0){
            fputs( "Sem_wait_err - Sem_A\n", stderr);
            return (void *)1;
        }
        if(sem_wait(&sem_B) != 0){
            fputs( "Sem_wait_err - Sem_B\n", stderr);
            return (void *)1;
        }

        sleep(1);

        if(sem_post(&sem_Module) != 0){
            fputs("Sem_post_err - Sem_Module\n", stderr);
            return (void *)1;
        }
        puts("Module is Ready");
    }
    return NULL;
}
void *widget(void *arg){
    while(!stop){
        if(sem_wait(&sem_Module) != 0){
            fputs("Sem_wait_err - Sem_Module\n", stderr);
            return (void *)1;
        }
        if(sem_wait(&sem_C) != 0){
            fputs("Sem_wait_err - Sem_C\n", stderr);
            return (void *)1;
        }

        sleep(1);

        puts("Widget is ready");
    }
    return NULL;
}

int main(){
    // INITIALIZATION
    int res;
    if((res = sem_init(&sem_A, 0, 0)) != 0){
        fputs("Init_err - sem_A\n", stderr);
        return EXIT_FAILURE;
    }
    if((res = sem_init(&sem_B, 0, 0)) != 0){
        fputs("Init_err - sem_B\n", stderr);
        return EXIT_FAILURE;
    }
    if((res = sem_init(&sem_C, 0, 0)) != 0){
        fputs("Init_err - sem_C\n", stderr);
        return EXIT_FAILURE;
    }
    if((res = sem_init(&sem_Module, 0, 0)) != 0){
        fputs("Init_err - sem_Module\n", stderr);
        return EXIT_FAILURE;
    }
    
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    // PTHREAD_CREATE
    pthread_t threads[5];
    if(pthread_create(&threads[0], NULL, detail_A, NULL) != 0){
        fputs("Create_err - pthread_detail_A\n", stderr);
        return EXIT_FAILURE;
    }
    if(pthread_create(&threads[1], NULL, detail_B, NULL) != 0){
        fputs("Create_err - pthread_detail_B\n", stderr);
        return EXIT_FAILURE;
    }
    if(pthread_create(&threads[2], NULL, detail_C, NULL) != 0){
        fputs("Create_err - pthread_detail_C\n", stderr);
        return EXIT_FAILURE;
    }
    if(pthread_create(&threads[3], NULL, module, NULL) != 0){
        fputs("Create_err - pthread_module\n", stderr);
        return EXIT_FAILURE;
    }
    if(pthread_create(&threads[4], NULL, widget, NULL) != 0){
        fputs("Create_err - pthread_widget\n", stderr);
        return EXIT_FAILURE;
    }

    if(stop){
        sem_post(&sem_A);
        sem_post(&sem_B);
        sem_post(&sem_C);
        sem_post(&sem_Module);
    }

    // PTHREAD JOIN
    int i;
    for(i = 0; i < 5; i++){
        res = pthread_join(threads[i], NULL);
        if(res != 0){
            fprintf(stderr, "Join_err - %d Thread \n", i);
            return EXIT_FAILURE;
        }
    }

    // DESTROYING
    if(sem_destroy(&sem_A) != 0){
        fputs( "Dest_err - sem_A\n", stderr);
        return EXIT_FAILURE;
    }
    if(sem_destroy(&sem_B) != 0){
        fputs( "Dest_err - sem_B\n", stderr);
        return EXIT_FAILURE;
    }
    if(sem_destroy(&sem_C) != 0){
        fputs( "Dest_err - sem_C\n", stderr);
        return EXIT_FAILURE;
    }
    if(sem_destroy(&sem_Module) != 0){
        fputs( "Dest_err - sem_Module\n", stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

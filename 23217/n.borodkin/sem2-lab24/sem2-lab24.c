#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

sem_t sem_A, sem_B, sem_C, sem_Module;
sem_t sem_stop;
volatile sig_atomic_t stop = 0;

pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

void handle_signal(int sig){
    stop = 1;

    sem_post(&sem_stop);

    sem_post(&sem_A);
    sem_post(&sem_B);
    sem_post(&sem_C);
    sem_post(&sem_Module);
}

void *signal_helper(void *arg){
    sem_wait(&sem_stop);
    pthread_mutex_lock(&cond_mutex);
    pthread_cond_broadcast(&cond_var);
    pthread_mutex_unlock(&cond_mutex);
    
    return NULL;
}

int wait_timeout(int seconds) {
    struct timespec ts;
    int res;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += seconds;

    pthread_mutex_lock(&cond_mutex);

    while (!stop) {
        res = pthread_cond_timedwait(&cond_var, &cond_mutex, &ts);
        if (res == ETIMEDOUT) {
            pthread_mutex_unlock(&cond_mutex);
            return 0;
        } else if (res != 0) {
            pthread_mutex_unlock(&cond_mutex);
            perror("pthread_cond_timedwait");
            return -1;
        }
    }
    pthread_mutex_unlock(&cond_mutex);

    return res;
}

void *detail_A(void *arg){
    while(!stop){
        wait_timeout(1);

        if(sem_post(&sem_A) != 0){
            perror("Sem_post_err - Sem_A");
            return (void *)1;
        }
        puts("Detail 'A' completed");
    }
    return NULL;
}

void *detail_B(void *arg){
    while(!stop){
        wait_timeout(2);

        if(sem_post(&sem_B) != 0){
            perror("Sem_post_err - Sem_B");
            return (void *)1;
        }
        puts("Detail 'B' completed");
    }
    return NULL;
}

void *detail_C(void *arg){
    while(!stop){
        wait_timeout(3);

        if(sem_post(&sem_C) != 0){
            perror("Sem_post_err - Sem_C");
            return (void *)1;
        }
        puts("Detail 'C' completed");
    }
    return NULL;
}

void *module(void *arg){
    while(!stop){
        if(sem_wait(&sem_A) != 0){
            perror("Sem_wait_err - Sem_A");
            return (void *)1;
        }
        if(sem_wait(&sem_B) != 0){
            perror("Sem_wait_err - Sem_B");
            return (void *)1;
        }

        wait_timeout(1);

        if(sem_post(&sem_Module) != 0){
            perror("Sem_post_err - Sem_Module");
            return (void *)1;
        }
        puts("Module is Ready");
    }
    return NULL;
}

void *widget(void *arg){
    while(!stop){
        if(sem_wait(&sem_Module) != 0){
            perror("Sem_wait_err - Sem_Module");
            return (void *)1;
        }
        if(sem_wait(&sem_C) != 0){
            perror("Sem_wait_err - Sem_C");
            return (void *)1;
        }

        wait_timeout(1);

        puts("Widget is ready");
    }
    return NULL;
}

int main(){
    int res;

    if((res = sem_init(&sem_A, 0, 0)) != 0){
        perror("Init_err - sem_A");
        return EXIT_FAILURE;
    }
    if((res = sem_init(&sem_B, 0, 0)) != 0){
        perror("Init_err - sem_B");
        return EXIT_FAILURE;
    }
    if((res = sem_init(&sem_C, 0, 0)) != 0){
        perror("Init_err - sem_C");
        return EXIT_FAILURE;
    }
    if((res = sem_init(&sem_Module, 0, 0)) != 0){
        perror("Init_err - sem_Module");
        return EXIT_FAILURE;
    }
    if((res = sem_init(&sem_stop, 0, 0)) != 0){
        perror("Init_err - sem_stop");
        return EXIT_FAILURE;
    }

    signal(SIGINT, &handle_signal);

    pthread_t threads[6];
    int err;

    err = pthread_create(&threads[0], NULL, detail_A, NULL);
    if(err != 0){
        fprintf(stderr, "Create_err - pthread_detail_A %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&threads[1], NULL, detail_B, NULL);
    if(err != 0){
        fprintf(stderr, "Create_err - pthread_detail_B %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&threads[2], NULL, detail_C, NULL);
    if(err != 0){
        fprintf(stderr, "Create_err - pthread_detail_C %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&threads[3], NULL, module, NULL);
    if(err != 0){
        fprintf(stderr, "Create_err - pthread_module %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&threads[4], NULL, widget, NULL);
    if(err != 0){
        fprintf(stderr, "Create_err - pthread_widget %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&threads[5], NULL, signal_helper, NULL);
    if(err != 0){
        fprintf(stderr, "Create_err - pthread_signal_helper %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    int i;
    for(i = 0; i < 6; i++){
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&sem_A);
    sem_destroy(&sem_B);
    sem_destroy(&sem_C);
    sem_destroy(&sem_Module);
    sem_destroy(&sem_stop);

    pthread_mutex_destroy(&cond_mutex);
    pthread_cond_destroy(&cond_var);

    return EXIT_SUCCESS;
}

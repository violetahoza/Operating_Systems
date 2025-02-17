#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>

sem_t regulate, block, max4;
sem_t *sem2_3, *sem5_2;

void *thread_function1(void *arg)
{
    int *val = (int *)arg;
    pthread_t thread;

    if (*val == 3)
        sem_wait(sem2_3);

    info(BEGIN, 2, *val);

    if (*val == 1)
    {
        int arg = 5;
        pthread_create(&thread, NULL, thread_function1, &arg);
        pthread_join(thread, NULL);
    }

    info(END, 2, *val);

    if (*val == 3)
        sem_post(sem5_2);

    return NULL;
}

void *thread_function2(void *arg)
{
    int *val = (int *)arg;

    if (*val == 2)
        sem_wait(sem5_2);

    info(BEGIN, 5, *val);

    info(END, 5, *val);

    if (*val == 6)
        sem_post(sem2_3);

    return NULL;
}

void *thread_function3(void *arg)
{
    int *val = (int *)arg;

    if (*val != 12)
    {
        sem_wait(&regulate);
        sem_wait(&max4);
    }

    info(BEGIN, 7, *val);

    if (*val != 12)
        sem_wait(&block);

    info(END, 7, *val);

    if (*val == 12)
    {
        for (int i = 0; i < 40; i++)
        {
            sem_post(&regulate);
            sem_post(&block);
        }
    }

    if (*val != 12)
        sem_post(&max4);

    return NULL;
}

int main()
{
    init();

    sem2_3 = sem_open("/p51", O_CREAT, 0644, 0);
    sem5_2 = sem_open("/p52", O_CREAT, 0644, 0);

    info(BEGIN, 1, 0);

    if (fork() == 0)
    {
        info(BEGIN, 2, 0);

        pthread_t p2_threads[5];
        int args[5];

        for (int i = 0; i < 4; i++)
        {
            args[i] = i + 1;
            pthread_create(&p2_threads[i], NULL, thread_function1, &args[i]);
        }

        if (fork() == 0)
        {
            info(BEGIN, 4, 0);
            info(END, 4, 0);
            return 0;
        }

        while (wait(NULL) > 0) // wait for all child processes to finish
            ;
        for (int i = 0; i < 4; i++)
            pthread_join(p2_threads[i], NULL);
        info(END, 2, 0);

        return 0;
    }

    if (fork() == 0)
    {
        info(BEGIN, 3, 0);
        info(END, 3, 0);
        return 0;
    }

    if (fork() == 0)
    {
        info(BEGIN, 5, 0);

        pthread_t p5_threads[6];
        int args[6];
        for (int i = 0; i < 6; i++)
        {
            args[i] = i + 1;
            pthread_create(&p5_threads[i], NULL, thread_function2, &args[i]);
        }

        if (fork() == 0)
        {
            info(BEGIN, 6, 0);
            if (fork() == 0)
            {
                info(BEGIN, 7, 0);

                pthread_t p7_threads[40];
                int args[40];

                sem_init(&block, 0, 0);
                sem_init(&max4, 0, 4);
                sem_init(&regulate, 0, 3);

                for (int i = 0; i < 40; i++)
                {
                    args[i] = i + 1;
                    pthread_create(&p7_threads[i], NULL, thread_function3, &args[i]);
                }
                for (int i = 0; i < 40; i++)
                    pthread_join(p7_threads[i], NULL);

                sem_destroy(&block);
                sem_destroy(&regulate);
                sem_destroy(&max4);

                info(END, 7, 0);
                return 0;
            }
            while (wait(NULL) > 0)
                ;
            info(END, 6, 0);
            return 0;
        }

        while (wait(NULL) > 0)
            ;
        for (int i = 0; i < 6; i++)
            pthread_join(p5_threads[i], NULL);
        info(END, 5, 0);

        sem_unlink("/p51");
        sem_unlink("/p52");

        return 0;
    }

    while (wait(NULL) > 0)
        ;
    info(END, 1, 0);

    return 0;
}
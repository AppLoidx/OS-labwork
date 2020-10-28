#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_t ntid;

void printids(char *s)
{
    pid_t pid;
    pthread_t tid;

    pid = getpid();
    tid = pthread_self();

    printf("%s pid %u tid %i (0x%x)\n", s, (unsigned int)pid, (unsigned int)tid, (unsigned int)tid);
}

void *
thr_fn(void * arg) {
    printids("Новый поток: ");
    return ((void *) 0);
}

int
main(void) {
    int err;

    err = pthread_create(&ntid, NULL, thr_fn, NULL);

    if (err != 0) {

        exit(err);
    }

    printids("Текущий поток: ");
    sleep(1);
    exit(0);
}

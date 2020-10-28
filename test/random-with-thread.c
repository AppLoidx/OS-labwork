#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_t ntid;


void print_rnd() {

    FILE * urandom = fopen("/dev/urandom", "r");
    unsigned int val;

    fread(&val, sizeof(unsigned int), 1, urandom);

    printf("Random value: %d", val);
}

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
    puts(" ");
    print_rnd();
    puts("");
    return ((void *) 0);
}

int
main(void) {
    int err;

    err = pthread_create(&ntid, NULL, thr_fn, NULL);

    if (err != 0) {

        exit(err);
    }

    printids("Новый поток: ");
    puts(" ");
    print_rnd();
    puts("");sleep(1);
    exit(0);
}

#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>

#define ALLOC_ADDR 0xB44603B3
#define THREADS_AMOUNT 3
#define FILL_MEM_SIZE 118
#define DUMP_FILE_SIZE 45

struct threadFuncArg {
    void * writeAddr;
    int size;
    FILE * dataSource;
    int logEnabled;
};

void*
threadFunc(void *restrictArg)
{
    struct threadFuncArg * arg = (struct threadFuncArg*) restrictArg;

    fread(arg->writeAddr, 1, arg->size, arg->dataSource);
    if (arg->logEnabled) {
        printf("Filling address: %p - pid: %u \t| tid: %lu\n", arg->writeAddr, getpid(), pthread_self());
    }

    pthread_exit((void *)0);
}

void
systemFree() {
    system("free");
    puts("");
}

// fills memory with mmap function
void *
fmem(void *addr, size_t size, int logEnabled)
{

    int err;
    
    if (logEnabled) {
        puts("\nBefore allocation:");
        systemFree();
    }

    void * mmapAddr;
    mmapAddr = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    
    if (logEnabled) {
        puts("\nAfter allocation: ");
        systemFree();
    }

    pthread_t threads [THREADS_AMOUNT] ;
    struct threadFuncArg * arg;
    
    
    for (int i = 0; i < THREADS_AMOUNT; i++) {
        
        arg = malloc(sizeof(arg));
        arg->size = size / THREADS_AMOUNT;
        arg->writeAddr = mmapAddr + i * ((arg->size / 8) + (arg->size % 8 > 0 ? 1 : 0));
        arg->dataSource = fopen("/dev/urandom", "r");
        arg->logEnabled = logEnabled;
        err = pthread_create(&threads[i], NULL, threadFunc, arg);

        if (err != 0)
        {
            puts("Error with creating thread");
            exit(err);
        }
   
    }

    for (int i = 0; i < THREADS_AMOUNT; i++) {
        err = pthread_join(threads[i], NULL);

        if (err != 0)
        {
            exit(err);
        }
    }

    if (logEnabled) {
        puts("\nAfter filling: ");
        systemFree();
    
        printf("Compare mmap returned addr: %p and map addr: %p \n", mmapAddr, addr);
    }

    return mmapAddr;
}

void
dumpMem(int fd, void * addr, int size) {
    
}

struct memoryDumpMap {
    int fd;
    void * addr;
    int size;
};

int main()
{
    int fillMemSize = FILL_MEM_SIZE * 1024 * 1024;
    puts("\nAllocating memory with mmap function");
    void * mmapAddr = fmem((void *)ALLOC_ADDR, fillMemSize, 1);

    munmap(mmapAddr, fillMemSize);

    puts("\nAfter deallocation");
    systemFree();

    
    void * memoryAddr = fmem((void *) ALLOC_ADDR, FILL_MEM_SIZE, 0);
    
    int dumpMemSize = DUMP_FILE_SIZE * 1024 * 1024;
    int filesAmount = fillMemSize / dumpMemSize + (fillMemSize % dumpMemSize > 0 ? 1 : 0);

    int files[filesAmount];
    struct memoryDumpMap* dumpMap[filesAmount];

    for (int i = 0; i < filesAmount; i++) {
        
        char filename[7];
        sprintf(filename, "dump.%d", i);
        files[i] = open(filename, O_RDWR | O_CREAT, (mode_t) 0600);

        if (files[i] == -1) {
            puts("Error: Can't create or open file");
        }

        dumpMap[i]->fd = files[i];
        dumpMap[i]->addr = (void*) ALLOC_ADDR;
        dumpMap[i]->size = dumpMemSize;
    }
    
    

    return 0;
}

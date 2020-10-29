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
#include <linux/futex.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/shm.h>
#include <inttypes.h>
#include <sched.h>

#define ALLOC_ADDR      0xB44603B3
#define THREADS_AMOUNT  3
#define FILL_MEM_SIZE   118
#define DUMP_FILE_SIZE  45
#define DATA_BLOCK_SIZE 11
#define COUNTERS_THREAD_AMOUNT 11 

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

int futex(int* uaddr, int futex_op, int val) {
      return syscall(SYS_futex, uaddr, futex_op, val, NULL, NULL, 0);
}

void wait_on_futex_value(int* futex_addr, int val) {
    futex(futex_addr, FUTEX_WAIT, val);
}

void wake_futex_blocking(int* futex_addr, int val) {
    while (1) {
        int futex_rc = futex(futex_addr, FUTEX_WAKE, val);
        if (futex_rc == -1) {
            perror("futex wake");
            exit(1);
        } else if (futex_rc > 0) {
            return;
        }
    }
}


void
dumpMem(int fd, void * addr, int size, int * futex) {
    wait_on_futex_value(futex, 0);
    *futex = 0;
    printf("Dump memory to %d\t from %p \t[size=%.2f MB]\n", fd, addr, size / (1024.0 * 1024.0));
    *futex = 1;
    int iterations = size / DATA_BLOCK_SIZE;
    int tenPercent = iterations / 10;
    int errno;
    int res = ftruncate(fd, 0);
    if (res == -1) {
        printf("truncate : %d\n", errno);
    }
    printf("Truncate file result: %d\n", res);
    for (int i = 0; i < iterations; i++) {
        if (i % tenPercent == 0) {
            printf("%d0%% - %.2f MB\n", i / tenPercent, (size - i * DATA_BLOCK_SIZE) / (1024.0 * 1024.0));
        }
        void * pointer = addr + i;
        size_t r = write(fd, &pointer, DATA_BLOCK_SIZE);
    }
}

void
counter(unsigned int times) {
    for (int i = times; i > 0; i--) {
        printf("...%d\n", i);
        sleep(1);
    }
}

struct memoryDumpMap {
    int fd;
    void * futex;
    void * addr;
    int size;
};

struct dumpTFArgs {
    struct memoryDumpMap* dumpMap[10];
    int filesAmount;
};

void *
dumpMemThreadFunc(void * arg) {
    struct dumpTFArgs * dumpArg = (struct dumpTFArgs *) arg;
    while (1) {
        for (int i = 0; i < dumpArg->filesAmount; i++) {
            struct memoryDumpMap * dumpMap = dumpArg->dumpMap[i];
            dumpMem(dumpMap->fd, dumpMap->addr, dumpMap->size, dumpMap->futex);
            puts("");
            sleep(1);
        }
    }
}

int
aggregateFile(struct memoryDumpMap * args) {
    int * futex = args->futex;
    wait_on_futex_value(futex, 0);
    *futex = 0;
    uint64_t sum = 0;
    
    if (lseek(args->fd, 0, SEEK_SET) == -1) {
        return -1;
    }

    unsigned char buf [DATA_BLOCK_SIZE];
    
    while(1) {
        
        size_t readBytes = read(args->fd, &buf, DATA_BLOCK_SIZE);
        
        if (readBytes == -1) {
            perror("Error file read");
            break;
        }

        if (readBytes < DATA_BLOCK_SIZE) {
            if (readBytes == 0) {
                break;
            }
        } else {
            for (int i = 0; i < DATA_BLOCK_SIZE; i++) {
                sum += buf[i];
            }
        }
    }

    printf("Calculated sum on file [%d]:\t %lu\n", args->fd, sum);

    *futex = 1;

    return 0;
}

void *
readFileFunc(void * arg) {
    struct dumpTFArgs * dumpArg = (struct dumpTFArgs *) arg;
    while(1) {
        int index = rand() % dumpArg->filesAmount;
        aggregateFile(dumpArg->dumpMap[index]);
        sleep(1);
    }
}


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

    struct dumpTFArgs * args = malloc(sizeof(struct dumpTFArgs));
    args->filesAmount = filesAmount;

    for (int i = 0; i < filesAmount; i++) {
        
        char filename[7];
        sprintf(filename, "dump.%d", i);
        files[i] = open(filename, O_RDWR | O_CREAT, (mode_t) 0600);

        if (files[i] == -1) {
            puts("Error: Can't create or open file");
        }
        args->dumpMap[i] = malloc(sizeof(struct memoryDumpMap));
        args->dumpMap[i]->fd = files[i];
        args->dumpMap[i]->addr = memoryAddr + i * (dumpMemSize / 8);
        args->dumpMap[i]->size = dumpMemSize;
        int shm_id = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0666);
        args->dumpMap[i]->futex = shmat(shm_id, NULL, 0);
    }
   
    
    pthread_t memDumpTP;
    pthread_attr_t * memDTA = malloc(sizeof(pthread_attr_t));
    struct sched_param * memDP = malloc(sizeof(struct sched_param));
    // memDP->sched_priority = 20;


    int res;
    res = pthread_attr_setschedparam(memDTA, memDP);
    res = pthread_create(&memDumpTP, NULL, dumpMemThreadFunc, args);
    puts("Writing thread created...");
    
    puts("Creating reading threads");
    pthread_t counters[COUNTERS_THREAD_AMOUNT];
     
    pthread_attr_t * memAttr = malloc(sizeof(pthread_attr_t));
    struct sched_param * memSP = malloc(sizeof(struct sched_param));
    // memSP->sched_priority = 25;

    res = pthread_attr_setschedparam(memAttr, memSP);
    for (int i = 0; i < COUNTERS_THREAD_AMOUNT; i++) {
        counters[i] = pthread_create(&counters[i], NULL, readFileFunc, args); 
    }

    printf("Created %d aggregator threads\n", COUNTERS_THREAD_AMOUNT);

    int times = 3;
    printf("Unlocking all blocks after %d seconds\n", times);
    counter(times);


    puts("Unlocking all blocks...");
    for (int i = 0; i < filesAmount; i++) {
        int * futex = args->dumpMap[i]->futex;
        wake_futex_blocking(futex, 1);
    }

    int err = pthread_join(memDumpTP, NULL);
    if (err != 0) {
        printf("Error pthred_join. Status: %d\n", err);
    }

    for (int i = 0; i < COUNTERS_THREAD_AMOUNT; i++) {
        pthread_join(counters[i], NULL);
    }

    return 0;
}

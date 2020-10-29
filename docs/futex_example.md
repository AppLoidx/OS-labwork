
```c
#include <stdio.h>
#include <pthread.h>
#include <linux/futex.h>
#include <syscall.h>
#include <unistd.h>

#define NUM 50

int futex_addr;

int futex_wait(void* addr, int val1){
  return syscall(SYS_futex,&futex_addr,val1, NULL, NULL, 0);
}
int futex_wake(void* addr, int n){
  return syscall(SYS_futex, addr, FUTEX_WAKE, n, NULL, NULL, 0);
}

void* thread_f(void* par){
        int id = (int) par;

    /*go to sleep*/
    futex_addr = 0;
    futex_wait(&futex_addr,0);

        printf("Thread %d starting to work!\n",id);
        return NULL;
}

int main(){
        pthread_t threads[NUM];
        int i;

        for (i=0;i<NUM;i++){
                pthread_create(&threads[i],NULL,thread_f,(void *)i);
        }

        printf("Everyone wait...\n");
        sleep(1);
        printf("Now go!\n");
    /*wake threads*/
    futex_wake(&futex_addr,50);

    /*give the threads time to complete their tasks*/
        sleep(1);


    printf("Main is quitting...\n");
        return 0;
}
```

Output:
```text
Everyone wait...
Now go!
Thread 0 starting to work!
Thread 1 starting to work!
Thread 2 starting to work!
Thread 3 starting to work!
Thread 4 starting to work!
Thread 5 starting to work!
Thread 6 starting to work!
Thread 7 starting to work
Thread 8 starting to work!
Thread 9 starting to work!
.
.
Main is quitting
```


### Explanation

1. You create 50 threads and put the main thread to sleep.
2. Within each thread, you set the value of futex_addr to zero (redundantly).
3. You call futex_wait with the address of that value, and a value parameter of zero. That means "block, if the value I point to is (still) really zero".
4. sys_futex checks that the value at &futex_addr is really zero, which is the condition for blocking your thread (that is important for the correct operability of the syscall, otherwise futex_wake would have to block alike NtReleaseKeyedEvent under Windows). Of course the value is zero, that's all any thread has ever written to it, so your thread blocks.
5. The main thread eventually returns from sleep and calls futex_wake with a parameter of 50, which means "wake (up to) 50 threads that are waiting on &futex_addr". So all your 50 threads wake up in one big thundering herd.
6. Each thread writes a message to stdout (without synchronization, so you might see garbled output) and exits.
7. The main thread exits as well, without joining threads or synchronizing, but due to the second call to sleep, chances are that it will "work fine" without evil things happening, anyway (not that it's a good thing to write code like this!).

Note that this method of waking N threads works, but is discouraged (much like using futex in the first place is discouraged, too). Typically you want to wake exactly one thread (use 1), or all threads (use INT_MAX).

Answer from : https://stackoverflow.com/questions/24488564/how-futex-works-in-this-case

By : [Damon](https://stackoverflow.com/users/572743/damon)


# OS laboratory work [60%]

> * Allocating memory with `mmap` and dumping memory to files.
> * System monitoring

### Run

```bash
make run
```

or compile sources:

```bash
gcc main.c -pthread
./a.out
```
### In strace we trust

Run script with strace:
```bash
./strace.sh
```

### Other files

Move to test directory

* [pthreadtest.c](test/pthreadtest.c) - prove that threads have the same pid [`make pthread-test`]
* [random.c](test/random.c) - work with `/dev/urandom` [`make random`]
* [random-with-thread.c](test/random-with-thread.c) - random + pthread [`make random-t`]



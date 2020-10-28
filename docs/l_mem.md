In a prior blog post, I talked about what virtual memory is, the difference between swapping and paging, and why it matters. (TL;DR: swapping is moving an entire process out to disk; paging is moving just specific pages out to disk, not an entire process. Running programs that require more memory than the system has will mean pages (or processes) are moved to/from disk and memory in order to get enough physical memory to run – and system performance will suck.)

Now I’ll talk about how to monitor virtual memory, on Linux (where it’s easy) and, next time, on Solaris (where most people and systems do it incorrectly.)

Assessing Linux Memory Usage

There are three things that may spring to mind when you think of measuring your memory system:

* how much physical memory is in use
* how much virtual memory is in use
* paging rate from one to the other
* Physical Memory

For physical memory usage, you can run top or free from a shell:
```text
[demo1.dc7:~]$ free -g
             total       used       free     shared    buffers     cached
             Mem:            47         45          1          0          0         21
```


or you can use your handy monitoring system to view this over time:

![](https://i.imgur.com/Z7iWuYg.png)

Some people may see the output of free, or the above graph, and react with “1G on my 48G system is all that’s free? I’m out of memory!”  This is a natural reaction – but wrong. Think of ‘free memory’ in Linux as ‘wasted memory’ (or better ‘memory the operating system has not yet been able to take advantage of’).  Almost half the memory on this system is in use: but by the file cache. The file cache does what it sounds like – caches in memory recently accessed files – meaning that if a program requests access to a file that is in the file cache, no disk access is required.  Linux uses all physical memory that is not needed by running programs as a file cache, for efficiency. But if programs need that physical memory, the kernel will reallocate the file cache memory to the programs. So memory used by the file cache is free (from the point of view of being available for allocation to programs) but serving a useful purpose until it is needed by a program.

Even if all Linux memory is used, and very little is free or in use as a file cache – that can be OK. It’s better to have some file cache in most situations: but not if you are running a large Java program, and want to maximize the heap; or a database, and you want the database to manage disk caching, not the OS (as the database has more knowledge about the utilization of the data.)

In any event, so long as there is free virtual memory, and not active swapping, you will be OK with highly utilized physical memory.

### Virtual Memory in Use 

To see the amount of swap memory in use, you can also use top or the free command

```text
[demo1.dc7:~]$ free -t
             total       used       free     shared    buffers     cached
Mem:      49376156   48027256    1348900          0     279292   22996652
-/+ buffers/cache:   24751312   24624844
Swap:      4194296          0    4194296
Total:    53570452   48027256    5543196
```

Or your monitoring:

![](https://i.imgur.com/NYwRanN.png)

So looking at the outputs above, we can see that the system has used zero swap space. So even though 90% of the total virtual memory space is in use (counting both swap and physical) there has never been a time when the system ran low enough on physical memory that it couldn’t free some from the file cache, and had to put some on swap.

If your swap usage is high – that can be dangerous, as it means the system is in danger of exhausting all the memory – and then if a program needs more, and is unable to get it, Bad Things happen. (Amongst others – the OOM (out of memory) killer will start to kill processes based on, among other criteria, the amount of memory they initially requested – which means that the server process that is the whole point of the server is likely to be one of the first to die.)

It should be noted, however, that a low to moderate level of usage of swap memory that is not actively being used is no cause for concern whatsoever. It just means the system has shifted pages not actively being used from physical memory to disk, to free memory for more active pages. This is a Good Thing. The key is to know whether the swap is being actively used, which brings us to the next section.

### Virtual Memory Paging Rate

To see the rate of memory pages being moved from physical memory to disk – use vmstat, and examine the columns si and so (which stands for pages swapped in, and swapped out).

e.g. on a system low on memory:

```text
dev1.lax6:~]$ vmstat 1
procs -----------memory---------- ---swap-- -----io---- --system-- -----cpu-----
r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
8 17 2422376 122428   2520  24436  952  676  1796   904 10360 4421 41  0 33 26  0
9 17 2423820 123372   2524  24316  732 1716   752  1792 12259 4592 43  0 25 32  0
8 17 2425844 120408   2524  25044  416 2204  1616  2264 14675 4514 43  0 36 21  0
7 19 2427004 120532   2568  25640  608 1280   764  1308 12592 4383 44  0 36 20  0
8 24 2428816 121712   2572  25688  328 1880   500  1888 13289 4339 43  0 32 25  0
```

![](https://i.imgur.com/ViALnCq.png)

This is really the main indicator that will show you memory issues – when your system is low on memory, it will swap out a lot of blocks. If this happens at a high rate, it can be a bottleneck to performance. Even worse, when your system needs to run code that is now on disk, instead of physical memory, it will have to swap that back in – which means your code is now running subject to the access time of disks (slow) compared to memory (fast.)

### Summary

The two metrics you need to care about are the rate of swapping (if it’s in the hundreds of blocks per second, for more than a few minutes – you are out of memory, and your system performance will suffer), and a high level of usage of swap (> 75% of swap space, not of total virtual memory.)  So long as you have a monitoring system alerting you to these two attributes, you are in good shape. And if you don’t – you’re playing with fire.



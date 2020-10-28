pthread-test: pthreadtest.o
	./pthreadtest.o
pthreadtest.o: pthreadtest.c
	gcc -Wall -o pthreadtest.o pthreadtest.c -pthread

random: random.o
	./random.o

random.o: random.c
	gcc -Wall -o random.o random.c

random-t: random-t.o
		./random-t.o

random-t.o: random-with-thread.c
	gcc -Wall -o random-t.o random-with-thread.c -pthread

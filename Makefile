run: main.o
	./main.o
main.o : main.c
	gcc -Wall -o main.o main.c -pthread

clean:
	rm *.o

run: main.o
	./main.o

main.o : main.c
	gcc -Wall -o main.o main.c -pthread



# Clean functions
#
# clean all
clean:
	make cbin cdump

# clean compiled sources
cbin:
	find . -type f -name '*.o' -delete

# clean mememory dumps
cdump:
	find . -type f -name 'dump.*' -delete

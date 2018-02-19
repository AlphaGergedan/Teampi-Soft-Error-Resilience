CC = mpiicpc

FLAGS = -Wall -std=c++11 -O0 -g3

all: libtmpi.so UnitTests 

libtmpi.so: tmpi.cpp tmpi.h
	$(CC) $(FLAGS) -fpic -c tmpi.cpp -o tmpi.o
	$(CC) -shared -Wl,-soname,libtmpi.so -o libtmpi.so tmpi.o

UnitTests: UnitTests.o libtmpi.so
	$(CC) $(FLAGS) UnitTests.o -o UnitTests -L. -ltmpi

UnitTests.o: UnitTests.cpp
	$(CC) $(FLAGS) -c UnitTests.cpp -o UnitTests.o

clean:
	rm UnitTests *.o *.so

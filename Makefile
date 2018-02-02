CC = mpiicpc

FLAGS = -Wall -std=c++11 -O0 -g3 -DLOGDEBUG

testdir = tests

all: HelloWorldTest PingPongTest NonBlockingTest libtmpi.so

NonBlockingTest: NonBlockingTest.o libtmpi.so
	$(CC) $(FLAGS) NonBlockingTest.o -o $(testdir)/NonBlockingTest -L. -ltmpi

HelloWorldTest: HelloWorldTest.o libtmpi.so
	$(CC) $(FLAGS) HelloWorldTest.o -o $(testdir)/HelloWorldTest -L. -ltmpi

PingPongTest: PingPongTest.o libtmpi.so
	$(CC) $(FLAGS) PingPongTest.o -o $(testdir)/PingPongTest -L. -ltmpi

libtmpi.so: tmpi.cpp tmpi.h
	$(CC) $(FLAGS) -fpic -c tmpi.cpp -o tmpi.o
	$(CC) -shared -Wl,-soname,libtmpi.so -o libtmpi.so tmpi.o

NonBlockingTest.o: NonBlockingTest.cpp
	$(CC) $(FLAGS) -c NonBlockingTest.cpp -o NonBlockingTest.o
	
HelloWorldTest.o: HelloWorldTest.cpp
	$(CC) $(FLAGS) -c HelloWorldTest.cpp -o HelloWorldTest.o

PingPongTest.o: PingPongTest.cpp
	$(CC) $(FLAGS) -c PingPongTest.cpp -o PingPongTest.o

clean:
	rm tests/* *.o *.so

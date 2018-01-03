CC = mpiicpc

FLAGS = -Wall -std=c++11

all: HelloWorldTest PingPongTest

HelloWorldTest: HelloWorldTest.o libtmpi.so
	$(CC) $(FLAGS) HelloWorldTest.o -o HelloWorldTest -L. -ltmpi

PingPongTest: PingPongTest.o libtmpi.so
	$(CC) $(FLAGS) PingPongTest.o -o PingPongTest -L. -ltmpi

libtmpi.so: tmpi.cpp tmpi.h
	$(CC) $(FLAGS) -fpic -c tmpi.cpp -o tmpi.o
	$(CC) -shared -Wl,-soname,libtmpi.so -o libtmpi.so tmpi.o
	
HelloWorldTest.o: HelloWorldTest.cpp
	$(CC) $(FLAGS) -c HelloWorldTest.cpp -o HelloWorldTest.o

PingPongTest.o: PingPongTest.cpp
	$(CC) $(FLAGS) -c PingPongTest.cpp -o PingPongTest.o

clean:
	rm -rf HelloWorldTest PingPongTest *.o *.so

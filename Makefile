CC = mpiicpc

FLAGS = -O2 -Wall

all: HelloWorldTest PingPongTest

HelloWorldTest: HelloWorldTest.o Wrapper.o
	$(CC) $(FLAGS) HelloWorldTest.o Wrapper.o -o HelloWorldTest

PingPongTest: PingPongTest.o Wrapper.o
	$(CC) $(FLAGS) PingPongTest.o Wrapper.o -o PingPongTest

Wrapper.o: Wrapper.cpp Wrapper.h
	$(CC) $(FLAGS) -c Wrapper.cpp -o Wrapper.o
	
HelloWorldTest.o: HelloWorldTest.cpp
	$(CC) $(FLAGS) -c HelloWorldTest.cpp -o HelloWorldTest.o

PingPongTest.o: PingPongTest.cpp
	$(CC) $(FLAGS) -c PingPongTest.cpp -o PingPongTest.o

clean:
	rm -rf HelloWorldTest PingPongTest *.o
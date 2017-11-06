CC = mpiicpc

FLAGS = -O2 -Wall

all: PingPongTest

PingPongTest: PingPongTest.o Wrapper.o
	$(CC) $(FLAGS) PingPongTest.o Wrapper.o -o PingPongTest

Wrapper.o: Wrapper.cpp Wrapper.h
	$(CC) $(FLAGS) -c Wrapper.cpp -o Wrapper.o

PingPongTest.o: PingPongTest.cpp
	$(CC) $(FLAGS) -c PingPongTest.cpp -o PingPongTest.o

clean:
	rm -rf PingPongTest *.o
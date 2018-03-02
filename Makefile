#CC = mpiicpc
#
#FLAGS = -Wall -std=c++11 -O0 -g3
#
#all: libtmpi.so UnitTests 
#
#libtmpi.so: tmpi.cpp tmpi.h
#	$(CC) $(FLAGS) -fpic -c tmpi.cpp -o tmpi.o
#	$(CC) -shared -Wl,-soname,libtmpi.so -o libtmpi.so tmpi.o
#
#UnitTests: UnitTests.o libtmpi.so
#	$(CC) $(FLAGS) UnitTests.o -o UnitTests -L. -ltmpi
#
#UnitTests.o: UnitTests.cpp
#	$(CC) $(FLAGS) -c UnitTests.cpp -o UnitTests.o
#
#clean:
#	rm UnitTests *.o *.so

	
.PHONY : clean


CC=mpiicpc
CFLAGS=-fPIC -g -Wall -std=c++11
LDFLAGS=-shared

SOURCES=$(shell echo *.cpp)
HEADERS=$(shell echo *.h)
OBJECTS=$(SOURCES:.cpp=.o)

TARGET = libtmpi.so

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET) : $(OBJECTS)
	mpiicpc $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
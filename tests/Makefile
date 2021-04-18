#CC=mpiicpc
CC=$(MPI_CXX)
CFLAGS += -g -Wall -std=c++11


TARGET = UnitTests
LIB = ../lib/libtmpi.so


all: $(LIB) $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@


$(TARGET) : UnitTests.o
	$(CC) $(CFLAGS) $^ -o $@ -L../lib -ltmpi

.PHONY : $(LIB)

$(LIB):
	make -C ../lib

.PHONY : clean

clean:
	rm -f UnitTests.o $(TARGET)
	make clean -C ../lib

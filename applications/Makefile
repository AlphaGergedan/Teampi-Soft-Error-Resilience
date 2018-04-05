CC=mpiicpc
CFLAGS += -g -Wall -std=c++11


TARGET = MasterWorker
LIB = ../lib/libtmpi.so


all: $(LIB) $(TARGET) 

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

	
$(TARGET) : MasterWorker.o
	$(CC) $(CFLAGS) $^ -o $@ -L../lib -ltmpi

.PHONY : $(LIB)

$(LIB):
	make -C ../lib

.PHONY : clean

clean:
	rm -f MasterWorker.o $(TARGET)
	make clean -C ../lib

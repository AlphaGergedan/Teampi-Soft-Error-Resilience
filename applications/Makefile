CC=mpiicpc
CFLAGS += -g -Wall -std=c++11


TARGETS = MasterWorker VectorAdd
LIB = ../lib/libtmpi.so


all: $(LIB) $(TARGETS) 

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

	
MasterWorker : MasterWorker.o
	$(CC) $(CFLAGS) $^ -o $@ -L../lib -ltmpi

VectorAdd : VectorAdd.o
	$(CC) $(CFLAGS) $^ -o $@ -L../lib -ltmpi

.PHONY : $(LIB)

$(LIB):
	make -C ../lib

.PHONY : clean

clean:
	rm -f *.o $(TARGETS)
	make clean -C ../lib

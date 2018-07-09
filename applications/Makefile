CC=mpiicpc
CFLAGS += -g -Wall -std=c++11
LFLAGS += -L../lib -ltmpi


TARGETS = bin/MasterWorker bin/VectorAdd bin/Latency bin/PerfSimulator
LIB = ../lib/libtmpi.so



all: $(LIB) $(TARGETS) 

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

	
bin/MasterWorker : bin/MasterWorker.o
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

bin/VectorAdd : bin/VectorAdd.o
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)
	
bin/Latency : bin/Latency.o
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

bin/PerfSimulator : bin/PerfSimulator.o
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

	
bin/MasterWorker.o : MasterWorker/MasterWorker.cpp
	$(CC) $(CFLAGS) -c $^ -o $@ $(LFLAGS)

bin/VectorAdd.o : VectorAdd/VectorAdd.cpp
	$(CC) $(CFLAGS) -c $^ -o $@ $(LFLAGS)
	
bin/Latency.o : Latency/Latency.cpp
	$(CC) $(CFLAGS) -c $^ -o $@ $(LFLAGS)

bin/PerfSimulator.o : PerfSimulator/PerfSimulator.cpp
	$(CC) $(CFLAGS) -c $^ -o $@ $(LFLAGS)

.PHONY : $(LIB)

$(LIB):
	make -C ../lib

.PHONY : clean

clean:
	rm -f bin/*.o $(TARGETS)
	make clean -C ../lib

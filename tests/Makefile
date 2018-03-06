CC=mpiicpc
CFLAGS += -g -Wall -std=c++11


TARGET = UnitTests
LIB = ../libtmpi.so


all: $(LIB) $(TARGET) 

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

	
$(TARGET) : UnitTests.o
	$(CC) $(CFLAGS) $^ -o $@ -L../ -ltmpi

.PHONY : $(LIB)

$(LIB):
	make -C ..

.PHONY : clean

clean:
	rm -f UnitTests.o $(TARGET)
	make clean -C ..

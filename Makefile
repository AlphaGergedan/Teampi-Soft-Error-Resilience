CC=mpiicpc
CFLAGS += -fPIC -g -Wall -std=c++11
LDFLAGS += -shared

SOURCES = $(shell echo src/*.cpp)
HEADERS = $(shell echo src/*.h)
OBJECTS = $(SOURCES:.cpp=.o)

TARGET = libtmpi.so

.PHONY : clean

all: $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

	
$(TARGET) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET) 

clean:
	rm -f $(OBJECTS) $(TARGET)

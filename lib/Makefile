CC=mpiicpc
CFLAGS += -fPIC -g -Wall -std=c++11
LDFLAGS += -shared

SRC = RankOperations.cpp Timing.cpp Wrapper.cpp
DEP = RankOperations.h Timing.h Wrapper.h TMPIConstants.h Logging.h
OBJECTS = $(SRC:.cpp=.o)

TARGET = libtmpi.so

.PHONY : clean

all: $(TARGET)

%.o: %.cpp $(DEP)
	$(CC) $(CFLAGS) -c $< -o $@

	
$(TARGET) : $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

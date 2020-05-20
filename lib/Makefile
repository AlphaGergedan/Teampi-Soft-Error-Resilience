CC=/opt/bin/mpic++
CFLAGS += -fPIC -g -Wall -std=c++11
LDFLAGS += -shared

#SRC = Rank.cpp RankControl.cpp Timing.cpp Wrapper.cpp teaMPI.cpp CommStats.cpp ErrorHandling.cpp
#DEP = Rank.h RankControl.h Timing.h Wrapper.h Logging.h teaMPI.h CommStats.h ErrorHandling/ErrorHandling.h
SRC := $(wildcard *.cpp) $(wildcard ErrorHandling/*.cpp)
DEP := $(wildcard *.h) $(wildcard ErrorHandling/*.h)
SRC := $(filter-out ErrorHandling/ErrorHandling.cpp, $(SRC))
DEP := $(filter-out ErrorHandling/ErrorHandling.h, $(DEP))

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

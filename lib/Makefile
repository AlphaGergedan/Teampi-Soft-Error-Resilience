CC=mpiicpc
CFLAGS += -fPIC -g -Wall -std=c++11 -DPAUSE_RANK
LDFLAGS += -shared

SRC = Rank.cpp RankControl.cpp Timing.cpp Wrapper.cpp
DEP = Rank.h RankControl.h Timing.h Wrapper.h Logging.h
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

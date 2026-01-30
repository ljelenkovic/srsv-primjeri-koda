CXX = g++
CXXFLAGS = -std=c++17 -O2 -pthread -Wall
LDFLAGS = -lrabbitmq

TARGET = stats
SOURCES = stats.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

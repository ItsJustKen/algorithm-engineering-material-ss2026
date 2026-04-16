CXX = g++
CXXFLAGS = -O2 -g -std=c++17
TARGET = particle_sim

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< -lm

profile: $(TARGET)
	perf record -F 99 --call-graph dwarf ./$(TARGET)
	perf report

clean:
	rm -f $(TARGET) perf.data perf.data.old

.PHONY: all profile clean

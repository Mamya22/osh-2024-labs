CC = g++
CFLAGS = -O2
VERSION = -std=c++11

TARGET = bubble_sort
SRCS = main.cpp bubblesort.cpp bubblesort.hpp

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(VERSION) -o $@ $^

clean:
	rm -f $(TARGET)

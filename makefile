CC=g++
CFLAGS=-c -std=c++11 -pthread
LDFLAGS=
CF=-std=c++11 -pthread
SOURCES=xbmain.cpp xbeeDMapi.cpp TTYserial.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=xbmain

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(CF) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o

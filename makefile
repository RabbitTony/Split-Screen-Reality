CC=g++
CFLAGS=-c -std=c++0x -pthread -lwiringPi -g
LDFLAGS=
CF=-std=c++0x -pthread -lwiringPi -g
SOURCES=xbmain.cpp xbeeDMapi.cpp TTYserial.cpp stopwatch.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=xbmain
HEADERS=xbeeDMapi.h TTYserial.h stopwatch.h xbmain.h
HEADEROBJS=$(HEADERS:.h=.o)


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(CF) -o $@

$(HEADEROBJS) : $(HEADERS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o

purge:
	rm -rf *.o
	rm $(EXECUTABLE)

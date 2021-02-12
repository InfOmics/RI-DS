INCLUDES= -I ./rilib/ -I ./include/
CC=g++
#CFLAGS=-c -O3 -g
CFLAGS=-c -O3

SOURCES= ri3.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ri351ds
#EXECUTABLE=ri351ds_matches
#EXECUTABLE=ri351ds_dbg


all:	$(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< $(INCLUDES) -o $@  

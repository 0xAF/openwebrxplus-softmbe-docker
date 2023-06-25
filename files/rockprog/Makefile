
## Created by Anjuta

CC = gcc
CFLAGS = -g -Wall -std=c99
OBJECTS = rockprog.o softrock.o
INCFLAGS = 
LDFLAGS = -Wl,-rpath,/usr/local/lib
LIBS = -lusb-1.0 -lpopt

all: rockprog

rockprog: $(OBJECTS)
	$(CC) -o rockprog $(OBJECTS) $(LDFLAGS) $(LIBS)

.SUFFIXES:
.SUFFIXES:	.c .cc .C .cpp .o

.c.o :
	$(CC) -o $@ -c $(CFLAGS) $< $(INCFLAGS)

count:
	wc *.c *.cc *.C *.cpp *.h *.hpp

clean:
	rm -f *.o

.PHONY: all
.PHONY: count
.PHONY: clean

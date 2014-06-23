CC = gcc
CFLAGS = -std=gnu99 -pipe -O2 -Wall -g -I/opt/local/include
LDFLAGS = -pipe -Wall -lm -lfftw3 -L/opt/local/lib

SOURCES = complex_proc.c
HEADERS = complex_proc.h
OBJECTS = $(SOURCES:.c=.o)
EXEC = cprtd

all: $(SOURCES) $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) 

.c.o: $(HEADERS)
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	rm -f *.o $(EXEC)


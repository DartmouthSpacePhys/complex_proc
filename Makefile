CC = gcc
CFLAGS = -std=gnu99 -pipe -O2 -Wall -g -I/opt/local/include
LDFLAGS = -pipe -Wall -lm -lfftw3 -L/opt/local/lib

SOURCES = complex_proc.c
SOURCES_1CH = complex_proc_1ch.c
HEADERS = complex_proc.h
OBJECTS = $(SOURCES:.c=.o)
OBJECTS_1CH = $(SOURCES_1CH:.c=.o)
EXEC = cprtd
EXEC_1CH = cprtd_1ch

all: $(SOURCES) $(EXEC) $(EXEC_1CH)

$(EXEC): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) 

$(EXEC_1CH): $(OBJECTS_1CH)
	$(CC) -o $@ $(OBJECTS_1CH) $(LDFLAGS) 


.c.o: $(HEADERS)
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	rm -f *.o $(EXEC)


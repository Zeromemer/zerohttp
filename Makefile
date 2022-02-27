exec = zerohttp
CC = gcc
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
cflags = -g -Wall
lflags = -lpthread

$(exec): $(objects)
	$(CC) $(objects) $(lflags) -o $(exec)

%.o: %.c include/%.h
	$(CC) -c $(cflags) $< -o $@

clean:
	-rm src/*.o

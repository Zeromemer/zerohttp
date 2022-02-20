exec = zerohttp
CC = gcc
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -g -Wall -lpthread


$(exec): $(objects)
	$(CC) $(objects) $(flags) -o $(exec)

%.o: %.c include/%.h
	$(CC) -c $(flags) $< -o $@

clean:
	-rm src/*.o

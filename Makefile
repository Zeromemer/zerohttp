exec=zerohttp
CC=gcc
sources=$(wildcard src/*.c)
objects=$(patsubst src/%.c, obj/%.o, $(sources))
cflags=-g -Wall
lflags=-lpthread

all: $(exec)

$(exec): $(objects)
	$(CC) $(objects) $(lflags) -o $(exec)

obj/%.o: src/%.c src/include/%.h
	$(CC) -c $(cflags) $< -o $@

clean:
	-rm obj/*

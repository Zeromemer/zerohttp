exec = zerohttp
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -g -Wall


$(exec): $(objects)
	gcc $(objects) $(flags) -o $(exec)

%.o: %.c include/%.h
	gcc -c $(flags) $< -o $@

install:
	make
	cp ./dgsh.elf /usr/local/bin/dgsh

clean:
	-rm src/*.o
